/*  shell & command processor for mpe
 *
 *  mpesh.c
 *  k theis 7/2020
 * 
 ************************************************************
 *
 *  This shell is part of the mpe operating system for
 *  the PMC-20 minicomputer. There are certain
 *  shared features with a unix-like shell. 
 *  mpe is related to /bin/sh but has no piping, redirects
 *  or programming functions (while, loop, variable 
 *  assignment etc) that you would expect from bash et al.
 *  
 *  There are two groups of users: 
 *  uid >= 2000 
 *  uid < 2000
 *  
 *  How this is used is currently not defined.
 *
 *
 *************************************************************
 *
 *  This program is licensed under the GNU license (since
 *  it uses gnu readline() which is also under the GNU
 *  license).
 *
 *  Please report problems to the github account where this
 *  is located or the system administrator for this computer.
 *
*/

#define TOKENBUF 10000      	// max length of expanded command line
#define EXECPATH "/cmds:./"		
#define RESTRICT_PATH "/cmds:./"
#define VER "mpesh version 4.04"
#define VERDATE __DATE__
#define LOGIN_COUNTER "/tmp/mpe_login_counter"  // goes up/down as users come/go
#define LOGIN_COUNTER_LOCK "/tmp/mpe.lock"  // if exists, wait till gone: in main()
#define NOLOGINS "/tmp/nologins"        // if exists, disallow logins
#define UP 1            // UP/DOWN used to inc/dec user counter in LOGIN_COUNTER
#define DOWN 0
#define RESTRICT_UID 2000   // uid >= number have a limited execution path

/* macro to free tokens, pointers and mem */
#define free_pointers free(args);wordfree(&p);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include "mpe.h"
#include <signal.h>
#include <libgen.h>
#include <wordexp.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>


/*
 * global vars 
 *
*/ 
int DEBUG = 0;      /* set as a command line option */
pid_t child_pid;    /* used for SIGINT in launch() */
char username [MAXLINE];    /* users log in directory */

/*
 * pre-define functions 
 *
*/ 
void changedir(char *);
int launch(char **);
int quit(void);
int write_counter(int);



/*
 *  use readline for command line 
 *  and history 
*/
static char *rline = (char *)NULL;
char *rl_gets(char *prompt) {

    /* if returning, free mem */
    if (rline) {
        free(rline);
        rline = (char *)NULL;
    }

    /* get a line of text */
    rline = readline(prompt);

    /* save if not empty */
    if (rline && *rline) add_history(rline);

    return rline;
}



/*
 *  Show internal commands
 *
*/  
typedef struct {
    char *name;
    char *help;
} COMMAND;
COMMAND commands[] = {
    {   "exit, lo", "Logoff and exit the system" },
    {   "prompt", "Toggle the prompt from short to long" },
    {   "commands", "Display this listing" },
    {   "ver", "Show the shell's current version & build date" },
    {   "pwd", "Show the current working directory" },
    {   "path", "Show the executable path" },
    {   "cls, clear", "Clear the screen (also ctrl-L)" },
    {   "cd", "Change directory" },
    {   "up", "Go up one directory level" },
    {   "pushd", "Save current directory on stack" },
    {   "popd", "Change to directory on stack, clears stack" },
	{	"exe [file]", "Set file to be executable." },
	{	"noexe [file]", "Set file to be non-executable." },
	{   "date", "Show the current time and date" },
    {   "logtime", "Show the connect time since login" },
    {   "help",  "Show external commands, help summary" },
    {   (char *)NULL, (char *)NULL }
};




/* signal handler for ctrl-c */
int kill_child(int signo, pid_t pid) {

    if (signo == SIGINT) {
        kill (child_pid,SIGINT);
    }
    return 0;
}


/* show fatal error and exit */
int fatal(char *error) {
    printf("fatal error:%s\n",error);
    exit(1);
}



/* ############ */
/* --- MAIN --- */
/* ############ */

int main(int argc, char **argv) {

char *linein;
char PROMPT[MAXLINE+1] = {"Ok> "};
char BUF[MAXLINE+1];    // for getcwd()
int prompt = 0, err = 0;
wordexp_t p;        // for wordexp
char **w;           // for wordexp
char **args;        // for wordexp et al
int i=0;            // wordexp, general purpose
int loggedin = 1;   // change to 0 when exiting
FILE *tmpfile;       // used by various
char instr[MAXLINE];    // used by various
char dirname[MAXLINE];  // for push/pop



    /* see if we are allowed to login:
     * if /tmp/nologins  exists, then
     * no logins are allowed.
     */
    if (!(access(NOLOGINS,F_OK))) {
            puts("No logins are allowed. Try back later.");
            exit(0);
    }

    
    
    /* get value from LOGIN_COUNTER and increment it at start.
     * at logoff, decrement it.
     * If LOGIN_COUNTER_LOCK exists wait until it disappears.
     *
     */
    i = 0;
    while (!(access(LOGIN_COUNTER_LOCK,F_OK))) {
        sleep(1);
        i++;
        if (i < 10) continue;
        puts("System is busy. Please try again later.");
        exit(0);
    }
    /* create lock file */
    tmpfile = fopen(LOGIN_COUNTER_LOCK,"w");
    if (!tmpfile) 
        fatal("Error creating lock file");
    fclose(tmpfile);
    
    /* read value of counter and increment it */
    if (write_counter(UP)) {
            unlink(LOGIN_COUNTER_LOCK);
            fatal("Cannot modify counter");
    }
    
    /* remove lock file */
    unlink(LOGIN_COUNTER_LOCK);
        

    /* minimal number of args to shell */
    if (argc == 2) {
        if (strcmp(argv[1],"debug")==0)
            DEBUG = 1;
		else
			printf("%s : unrecognized option\n",argv[1]);
	}

    /* set execute path */
    char pathName[MAXLINE];
    uid_t uid;
    uid = getuid();
    //strcpy(pathName,"PATH=");
    if (uid < RESTRICT_UID)
        strcat(pathName,EXECPATH);
    else 
        strcat(pathName,RESTRICT_PATH);
    unsetenv("PATH");
	if (err != 0) perror("unsetenv");
	err = setenv("PATH",EXECPATH,1);
	if (err != 0) perror("setenv");

    /* get user name (used in cd) */
    strcpy(username,getenv("LOGNAME"));
    if (strcmp(username,"root")==0) 	// must be using a chroot jail
        strcpy(username,getenv("SUDO_USER"));
    strcpy(BUF,"/users/");
    strcat(BUF,username);
    if (chdir(BUF) != 0) 
        printf("Home directory %s unavailable\n",BUF);

    printf("Welcome %s\n",username);

	/* set some environment vars */
	putenv("SHELL=/cmds/mpesh");

    /* ignore ctrl-c (re-enabled elsewhere) */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) 
        perror("signal ");

    /* get login time */
    time_t logintime, curtime, starttime, endtime, cputime;
    time(&logintime);   // seconds since user started the shell

    /* if remote login, get, show info */
    char *varname;
    varname = "SSH_CONNECTION";
    if (getenv(varname))
        printf("Remote connection from %s\n",getenv(varname));

	/* if /system/logmsg exists, display it */
	if (!access("/system/logmsg",F_OK)) {
		tmpfile = fopen("/system/logmsg","r");
		if (tmpfile) {
			while(1) {
				fgets(BUF,MAXLINE,tmpfile);
				if (feof(tmpfile)) break;
				printf("%s%s",_green,BUF);
			}
			fclose(tmpfile);
			//white();
			printf("%s\n",_white);
			printf("\n");
		}
	}

	/* set some vars */
	err = unsetenv("PWD");
	if (err != 0) perror("unsetenv");
	getcwd(BUF,MAXLINE);
	err = setenv("PWD",BUF,1);
	if (err != 0) perror("setenv");


	err = unsetenv("HOME");
	if (err != 0) perror("unsetenv");
	err = setenv("HOME","/users/guest",1);
	if (err != 0) perror("setenv");


    while (loggedin) {
/* ------------------------------- */
/* #     main command loop       # */
/* ------------------------------- */

    /* set the prompt */
    if (!prompt) strcpy(PROMPT,"Ok> ");
    if (prompt) {
        strcpy(PROMPT,getcwd(BUF,MAXLINE));
        strcat(PROMPT," Ok> ");
    }

    linein = rl_gets(PROMPT);
    if (linein[0] == '\0') continue;

    /* token splitting, globbing and quote removal */
    err = wordexp(linein, &p, 0);
    if (err != 0) {
        if (err == WRDE_SYNTAX)  puts("sh: character mismatch or unbalanced quotes");
        if (err == WRDE_NOSPACE) puts("sh: memory error");
        if (err == WRDE_BADVAL)  puts("sh: bad shell variable");
        if (err == WRDE_BADCHAR) puts("sh: invalid special character");
        printf("%s\n",linein);
        continue;
    }

    /* convert to tokens */
    w = p.we_wordv;
    args = malloc(TOKENBUF);
    if (!args) fatal("sh: memory allocation error");
    for (i=0; i<p.we_wordc; i++)
        args[i] = w[i];
    args[i] = (char *)NULL;

    /* ignore comments in batch mode */
    if (strcmp(args[0],"#")==0) continue;


    /* ------------------- */
    /* see what user wants */
    /* ------------------- */

    /* reload the shell (valuable while testing) */
    if (strcmp(args[0],"reload")==0) {
        write_counter(0);
        execv("/cmds/sh",NULL);	// 			NOTE: ### SYSTEM DEPENDANT ###
    }

    /* log off/exit the shell */
    if ((strcmp(args[0],"exit")==0) || (strcmp(args[0],"lo")==0)) {
        int d,h,m,s;
        s = time(&curtime) - logintime;     // show connected time
        m = s/60; h = m/60; d = h/24;
        s = s%60; m = m%60; h = h%24; d = d%60;
        printf("\nlogged in for %dd days %02d:%02d:%02d \n",d,h,m,s);
        printf("Goodbye\n");
        loggedin = 0;
        break;
    }

    /* clear the screen (ctrl-l also works) */
    if ((strcmp(args[0],"clear")==0) || (strcmp(args[0],"cls")==0)) {
        const char* blank = "\e[1;1H\e[2J";
        write(STDOUT_FILENO,blank,12);
        free_pointers;
        continue;
    }

    /* show connect time */
    if (strcmp(args[0],"logtime")==0) {
        int d,h,m,s;
        s = time(&curtime) - logintime; // show connect time for user
        m = s/60; h = m/60; d = h/24;
        s = s%60; m = m%60; h = h%24; d = d%60;
        green();
        printf("\nlogged in at (UTC) %s",ctime(&logintime));
        printf("connect time: %dd days %02d:%02d:%02d \n",d,h,m,s);
        free_pointers;
        white();
        continue;
    }

	/* set file executable */
	if (strncmp(args[0],"exe",3)==0) {
		chmod(args[1],S_IRWXU);
		free_pointers;
		continue;
	}

	/* unset file executable */
	if (strncmp(args[0],"noexe",5)==0) {
		chmod (args[1],S_IRUSR|S_IWUSR);
		free_pointers;
		continue;
	}

    /* show current working directory */
    if (strcmp(args[0],"pwd")==0) {
        green();
        printf("%s \n",getcwd(BUF,MAXLINE));
        white();
        free_pointers;
        continue;
    }

    /* show execute path */
    if (strcmp(args[0],"path")==0) {
        char *ppath;
        ppath = getenv("PATH");
        green();
        puts(ppath);
        white();
        free_pointers;
        continue;
    }

    /* go up 1 directory level */
    if (strcmp(args[0],"up")==0) {
        chdir("..");    // add restriction for RESTRICT_UID
        free_pointers;
        continue;
    }

    /* expand/contract prompt */
    if (strcmp(args[0],"prompt")==0) {
        prompt = abs(prompt-1);
        free_pointers;
        continue;
    }

    /* show version */
    if (strcmp(args[0],"ver")==0) {
        green();
        printf("%s %s\n",VER,VERDATE);
        white();
        free_pointers;
        continue;
    }

    /* list internal commands */
    if (strcmp(args[0],"commands")==0) {
        //green();
        for (int i=0; commands[i].name; i++) 
            printf("%s%-*s %-*s \n",_green,15,commands[i].name,20,commands[i].help);
        //white();
        printf("%s\n",_white);
        free_pointers;
        continue;
    }

    /* change directory */
    if (strcmp(args[0],"cd")==0) {
        changedir(args[1]);
        free_pointers;
        continue;
    }

    /* pushd: save the current directory name */
    if (strcmp(args[0],"pushd")==0) {
        strcpy(dirname,getcwd(BUF,MAXLINE));
        free_pointers;
        continue;
    }

    /* popd: change to the dir saved by pushd */
    if (strcmp(args[0],"popd")==0) {
        if (dirname[0]=='\0') {
            puts("no directory on stack");
            free_pointers;
            continue;
        }
        changedir(dirname);
        memset(dirname,0,MAXLINE-1);
        free_pointers;
        printf("changed to %s \n",getcwd(BUF,MAXLINE));
        continue;
    }

    /* nothing else to do: run external command */
    err = launch(args);
    if (err) puts("execution error");
    free_pointers; 
    continue;

    /* command not found (shouldn't get here) */
    //red();
    printf("%ssh: line not caught",_red);
    printf("%s\n",_white);
    free_pointers;
    continue;

    } // done with main command loop

    /* user logged out */
    free_pointers;
    //white();
	printf("%s\n",_white);

    /* read value of counter and decrement it 
     *
     * If LOGIN_COUNTER_LOCK exists wait until it disappears.
     *
     */
    i = 0;
    while (!(access(LOGIN_COUNTER_LOCK,F_OK))) {
        sleep(1);
        i++;
        if (i < 10) continue;
        puts("System is busy. Logging Out.");
        exit(0);
    }
    /* create lock file */
    tmpfile = fopen(LOGIN_COUNTER_LOCK,"w");
    if (!tmpfile) 
        fatal("Error creating lock file");
    fclose(tmpfile);


    /* read value of counter and decrement it */
    if (write_counter(DOWN)) {
            unlink(LOGIN_COUNTER_LOCK);
            fatal("Cannot modify counter");
    }

    /* remove lock file */
    unlink(LOGIN_COUNTER_LOCK);
   
    /* done */
    quit();

}
/* -------- End of Main ---------- */


/* #################### */
/* ----- ROUTINES ----- */
/* #################### */


/* removeChar
 *
 *
 * remove a char from a line
 *
*/ 
void removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}




/*  LAUNCH
 *
 *  run external command 
 *
*/  
int launch(char **args) {
    pid_t pid, wpid;
    int status = 0;
    int i=0;

    if (args[0] == NULL) {
        printf("%slaunch: no args supplied%s",_red,_white);
        return 1;
    }


    if (DEBUG) {
        for (i=0; args[i]; i++)
            if (args[i]) printf("launch: args[%d]=[%s] len=%d\n",i,args[i],strlen(args[i]));
    }

    /* enable ctrl-c */
    if (signal(SIGINT, SIG_DFL) == SIG_ERR)
        perror("signal ");

    pid = fork();
    if (pid < 0) {      // fork failed
        perror("external command error ");
        return 1;
    }

    if (pid == 0) {     // child process: exec the command w/args
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
        }
        // child finished: close gracefully
        exit(EXIT_FAILURE);
    } else {       // parent process
        
        /* ctrl-c kills child */
        child_pid = pid;
        signal(SIGINT,(void (*)(int))kill_child);
        
        do {     // wait until the child exits
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    
    if (WIFSIGNALED(status))    // handle segfaults etc
        printf("killed, status %d \n",WTERMSIG(status));

    //else if  (WIFEXITED(status))  // normal exit 
    //    printf("exited, status %d\n", WEXITSTATUS(status));
    
    else if  (WIFSTOPPED(status)) 
        printf("stopped by signal %d \n", WSTOPSIG(status));
   
    /* ignore ctrl-c */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
        perror("signal ");

    return 0;
}







/*  CHDIR
 *
 *  change working directory
 *
*/  
void changedir(char *linein) {
uid_t uid;
uid = getuid();
struct passwd *pw;
pw = getpwuid(uid);
char homedir[MAXLINE];
char envvar[MAXLINE];
int err = 0;
char BUF[MAXLINE];

    /* cd by itself, go to home directory */
    if (linein==NULL) {
        strcpy(homedir,"/users/");
        strcat(homedir,username);
        if (chdir(homedir) != 0) {
            perror("");
			return;
		}
		/* set env var PWD */
		err = unsetenv("PWD");
    	if (err != 0) perror("unsetenv");
    	getcwd(BUF,MAXLINE);
    	err = setenv("PWD",BUF,1);
    	if (err != 0) perror("setenv");
		return;
        
    }

    /* cd to supplied path */
    if (chdir(linein) != 0) {
        perror("");
		return;
	}
	/* set env var PWD */
	err = unsetenv("PWD");
    if (err != 0) perror("unsetenv");
    getcwd(BUF,MAXLINE);
    err = setenv("PWD",BUF,1);
    if (err != 0) perror("setenv");
    return;

}



/*  
 *  exit the shell/system
 *
*/  
int quit(void) {
    void rl_clear_history (void);       // clear readline history
    exit(0);
}




/*  write_counter
 *
 *  update the user login counter in/tmp
 *  (used for system overhead, user control
 *  in multi-user scenerio)
 *
*/  
int write_counter(int updn) {   // updn: 1=add, 0=subtract
FILE *tmpfile;
int i = 0;
char cnt[20];
int fd;

    /* create lock file */
    tmpfile = fopen(LOGIN_COUNTER_LOCK,"w");
    if (!tmpfile) 
        fatal("Error creating lock file");
    fclose(tmpfile);


    /* read value of counter and increment it */
    tmpfile = fopen(LOGIN_COUNTER,"r");
    if (!tmpfile) {
        unlink(LOGIN_COUNTER_LOCK);
        puts("Error updating login counter");
    }
    fgets(cnt,MAXLINE-1,tmpfile);
    i = atoi(cnt);
    if (updn) 
        i += 1;
    else
        i -= 1;
    if (i < 0) i = 0;   // no underflow
    fclose(tmpfile);
    



    /* write updated value to counter */
    fd = open(LOGIN_COUNTER,O_WRONLY|O_CREAT,0777);
    if (fd==-1) {
        unlink(LOGIN_COUNTER_LOCK);
        fatal("Error writing to login counter");
    }
    sprintf(cnt,"%d",i);
    write(fd,cnt,5);
    close(fd);


    /* remove lock file */
    unlink(LOGIN_COUNTER_LOCK);

    return 0;
}


