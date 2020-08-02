/*
 * mpesh - minimal shell 
 *
 * 6/25/2020
 * k theis
 *
 * NOTE: many commands and functions are locked down for users
 * with a uid >= UIDRESTRICT. Example: the basic program has a sys() call
 * that runs external commands. We don't want guests to run these.
 * 
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <stdint.h>
#include <sys/statvfs.h>


#ifndef MAXLINE
#define MAXLINE 80
#endif

#define ver "MPE ver 4.01 on the PMC-20 \ncompile date: "
#define verdate __DATE__

#define EDITOR "/usr/local/bin/led"     // line editor
#define BASIC "/usr/local/bin/basic"    // basic interpreter
#define UIDRESTRICT 2000                // uid's >= this number are restricted
#define EXECPATH "/bin/:/usr/bin/:/usr/local/bin/:./"  // path for command searches
#define REXECPATH "/usr/local/bin"      // path for restricted users
#define NOIDLE 0
#define NOAUTH "not authorized"         // message when my_uid >= UIDRESTRICT



/* global vars */
char PROMPT[] = {"Ok> \0"};
char ER[] = {"ER, \0"};
char linein[MAXLINE];

/* get user's id etc from the password file */
char my_home[MAXLINE]={"none"};
char my_name[MAXLINE]={"none"};
int my_uid = -1;         // user ID (uid)

int longprompt = 0;     // 0:short prompt, 1=long prompt (show cwd)
int exit_signal = 0;    // runs while 0, 999: exit program

/* --- define internal command list --- */
char INTERNAL[][MAXLINE] = {
    "        --- Command Summary ---   \0",
    "exit, logout, lo:  logoff and exit \0",
    "cls  ctrl-l:       clear the screen \0",
    "commands:          display shell commands \0",
    "help:              show this help file \0",
    "ver:               show mpe version information \0",
    "prompt:            toggle between long/short prompt* \0",
    "date:              show current date/time \0",
    "times:             show login times \0",
    "conv [value] [base]:  convert [value] (base 2-36) of base [base] to decimal \0",
    "type [filename]:   display a text file a page at a time \0",
    "filesize [filename]:  show size of filename \0",
    "stat [filename]:   display details on filename (ext cmd)\0",
    "dir:               display directory contents \0",
    "cd [name]:         jump to directory name \0",
    "up:                go up one level from the current directory \0",
    "down [name]:       go down to directory \0",
    "home:              change directories to your login directory \0",
    "create [name]:     create a new subdirectory* \0",
    "new [filename]:    create a small/empty textfile \0",
    "delete [name]:     delete a subdirectory or file* \0",
    "copy [oldfile][newfile]:  copy oldfile to newfile* \0",
    "rename [oldfilename][newfilename]: rename a file/directory* \0",
    "readonly [filename]:   set [filename] to read only \0",
    "readwrite [filename]:  set [filename] to read & write \0",
    "run [filename]:    execute the file [filename] in the current directory* \0",
    "basic [filename]:  run (optional) [filename] in the basic interpreter* \0",
    "edit [filename]:   invoke the led line editor with [filename]* \0",
    "concat [file1][file2]: concatinate file2 onto file1* \0",
    "filesort [filename]:   sort the file [filename]* \0",
    "status:            show user and system statistics \0",
    " \0",
    "* - access restricted by user\0",
    "//EXIT//\0"
    };

char CMD[][MAXLINE]={
    "exit","logout","lo","cls","ctrl-l","help","commands","ver",
    "prompt","path","date","times","conv","type","filesize","dir",
    "cd","up","down","home","created","createf","newd","newf","reload",
    "del","delete","copy","rename","public","private","readonly",
    "readwrite","run","basic","edit","concat","filesort","status",
    "//EXIT//\0"
    };


/* predefine routines */
int quit(void);
void cls(void);
void dir(char *);
void changedir(char *);
void chdirup(char *);
void createdir(char *);
void delete(char *);
int is_regular_file(const char *);
int isDirectory(const char *);
int testid(void);
void createfile(char *);
char **mpe_split_line(char *);
int mpe_execute(char **);
int mpe_launch(char **);
void type(char *);
void copy(char *);
void renamefile(char *);
void concat(char *);
void filesort(char *);
void filesize(char *);
void readonly(char *);
void readwrite(char *);
void conv(char *);
void public(char *);
void private(char *);
void status(char *);


/* ##### split args ##### */
void tree(char *linein) {
    char **args = mpe_split_line(linein);
    int argc=0;
    while (args[argc++]!=NULL);
    argc--;
    
    printf("argc %d\n",argc);
    while (--argc > 0)
           printf("%s ",*++args);
    printf("\n");
   return;
}


/********* qsort compare routine **********/
static int cmpstringp(const void *p1, const void *p2) {     // for qsort
    //printf("comparing %s  %s\n",(char *)p1,(char *)p2);
    return strcmp((char *) p1, (char *) p2);        // alpha compare
}



/******** readline routines ***********/
/* A static variable for holding the line. */
/* used to read a command line */
static char *line_read = (char *)NULL;

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *rl_gets (char *prompt) {
  
    /* if returning, free mem 1st */
    if (line_read) {
      free (line_read);
      line_read = (char *)NULL;
    }

    /* Get a line from the user. */
    line_read = readline (prompt);

    /* If the line has any text in it, save it on the history. */
    if (line_read && *line_read)
        add_history (line_read);

    return (line_read);
}


/* ########################### */
/* -------- SIGNALS ---------- */
/* ########################### */

void termination_handler (int signum) {
    /* sigterm caught - cleanup, close mpesh */
    if (NOIDLE) 
        exit_signal = 0;
    else {
        fprintf(stdout,"Session Closing: forced logout\n");
        fflush(stdout);
        quit();
    }
}

void ctrlc_handler (int signum) {
    /* ctrl-c caught ignore */
    exit_signal = 0;
}

void seg_handler (int signum) {
    /* SIGSEGV caught - segfault */
    fprintf(stdout,"Memory Error sig11\n");
    abort();
}




/* ################### */
/* ------ MAIN ------- */
/* ################### */

int main(int argc, char **argv) {

int err = 0;
char *line;
char lPROMPT[MAXLINE];
/* used for prompt() */
int cursize = 1000;
char curdir[cursize];

/* get user's id etc from the password file */
//char my_home[MAXLINE]={"none"};
//char my_name[MAXLINE]={"none"};

struct passwd *pw;
uid_t uid;
uid = geteuid ();
pw = getpwuid (uid);
    if (pw) {
        my_uid = pw->pw_uid;
        strncpy(my_name,(char *)pw->pw_name,MAXLINE-1);
        strncpy(my_home,(char *)pw->pw_dir,MAXLINE-1);
    }
    else {
        printf("User name not in the system\n");
        exit(1);
    }
    /* test if not correctly logged in */
    if (my_uid == -1) {
        printf("User id not in the system\n");
        exit(1);
    }

/* get login time (in seconds) */
time_t logintime, curtime, starttime, endtime, cputime;
time(&logintime);   // holds second count when user logged in

/* get connect info */
char *varname;
varname = "SSH_CONNECTION";
/* if remote, show connection */
if (getenv(varname) != NULL)
    printf("Remote Connection from %s\n",getenv(varname));

/* set execute path for the shell */
char path[100]="PATH=";
char *pathName;
if (my_uid < UIDRESTRICT) 
    pathName = EXECPATH;
else
    pathName = REXECPATH;
putenv(strcat(path, pathName));
/* see path command (below) for more info */


/* ################### */
/* #### MAIN LOOP #### */
/* ################### */

mainloop:
/* test if uid is root (uid=0) */
    if (my_uid == 0) {
        printf("user root cannot run this shell\n");
        exit(1);
    }

    /* ### catch signals ### */
    
    /* ctrl-c */
    if (signal(SIGINT, ctrlc_handler) == SIG_IGN) {
        signal(SIGINT,SIG_IGN);
        printf("\n");
        /* do nothing for now */
        //if (exit_signal == 0) goto mainloop;
    }
    
    

    /* term */
    if (signal(SIGTERM, termination_handler) == SIG_IGN) {
        signal(SIGTERM, SIG_IGN);
        /* will close from the signal handler code (above) */
    }


    /* clear the entry buffer first */
    memset(linein,0,MAXLINE);

    /* show prompt */
    if (longprompt) sprintf(lPROMPT,"%s %s",getcwd(curdir,cursize),PROMPT);  // show long prompt
    if (!longprompt) sprintf(lPROMPT,"%s",PROMPT);       // show short prompt

    /* use readline() (for history) */
    if (line) line = NULL;
    line = rl_gets(lPROMPT);
    strcpy(linein,line);
    if (linein[0]=='\0') goto mainloop;

    /* ----------------------------------------------- */
    /* ---------- internal command tests ------------- */
    /* ----------------------------------------------- */

    /* ##################### */
    /* ### Misc Commands ### */
    /* ##################### */

    if ((strcmp(linein,"logout")==0) ||     // exit the system 
        (strcmp(linein,"logoff")==0) ||
        (strcmp(linein,"lo")==0) ||
        (strcmp(linein,"exit")==0)) {
        int d,h,m,s;
        s = time(&curtime) - logintime;     // show connected time
        m = s/60; h = m/60; d = h/24;
        s = s%60; m = m%60; h = h%24; d = d%60;
        printf("\nlogged in for %dd days %02d:%02d:%02d \n",d,h,m,s);
        printf("Goodbye\n");
        quit();
    }

    if (strcmp(linein,"cls")==0) {      // clear the screen (ctrl-l does too)
        cls();
        goto mainloop;
    }
    
    if (strncmp(linein,"help",4)==0) {      // show internal commands help
        int n=0;
        while (1) {
            if (strcmp(INTERNAL[n],"//EXIT//")==0) {
                printf("\n");
                goto mainloop;
            }
            printf("%s\n",INTERNAL[n++]);
        }
    }
   
    if (strncmp(linein,"commands",8)==0) {  // show shell commands
        int n=0;
        printf("\n");
        while (1) {
            for (int x=0; x<8; x++) {
                if (strcmp(CMD[n],"//EXIT//")==0) {
                    printf("\n");
                    goto mainloop;
                }
                printf("%s ",CMD[n++]);
            }
            printf("\n");
        }
    }

    if (strcmp(linein,"path")==0) {     // show execute path
        char* pPath;
        pPath = getenv("PATH");
        printf("\n%s\n", pPath);
        goto mainloop;
    }

    if (strcmp(linein,"times")==0) {
        int d,h,m,s;
        s = time(&curtime) - logintime; // show connect time for user
        m = s/60; h = m/60; d = h/24;
        s = s%60; m = m%60; h = h%24; d = d%60;
        printf("\nlogged in at %s",ctime(&logintime));
        printf("connect time: %dd days %02d:%02d:%02d \n",d,h,m,s);
        goto mainloop;
    }

    if (strcmp(linein,"prompt")==0) {   // toggle between long/short prompt 
        longprompt = abs(longprompt-1);
        goto mainloop;
    }

    if (strncmp(linein,"conv",4)==0) {    // show given number as decimal
        conv(linein);
        goto mainloop;
    }

    if (strncmp(linein,"status",6)==0) {   // show status 
        status(linein);
        goto mainloop;
    }

    if ((strncmp(linein,"ver",3)==0) || (strncmp(linein,"uname",5)==0))  {
        printf("\n%s %s\n",ver,verdate);
        goto mainloop;
    }

    if (strcmp(linein,"date")==0) {     // show time/date
        char outstr[200];
        time_t t;
        struct tm *tmp;

        t = time(NULL); 
        tmp = localtime(&t);
        if (tmp == NULL) {
            perror("localtime");
                goto mainloop;;
        }
        if (strftime(outstr, sizeof(outstr), "%A %D %I:%M %p", tmp) == 0) {
           fprintf(stderr, "strftime returned 0");
               goto mainloop;
        }
        printf("\n%s\n", outstr);
        goto mainloop;
    }

    /* ########################### */
    /* ### Filesystem Commands ### */
    /* ########################### */

    if (strncmp(linein,"readonly",8)==0) {  // set file to read only permissions
        readonly(linein);
        goto mainloop;
    }

    if (strncmp(linein,"readwrite",9)==0) {    // set file to read/write perms
        readwrite(linein);
        goto mainloop;
    }
    
    if (strncmp(linein,"public",6)==0) {    // set file perms to public 
        public(linein);
        goto mainloop;
    }

    if (strncmp(linein,"private",7)==0) {   // set file perms to private
        private(linein);
        goto mainloop;
    }

    if (strncmp(linein,"filesize",8)==0) {  // show file size bytes/blocks
        filesize(linein);
        goto mainloop;
    }

    if (strncmp(linein,"dir",3)==0) {      // show the current directory, contents
        dir(linein);
        goto mainloop;
    }
    
    if ((strncmp(linein,"chdir",5)==0) ||       // change directories
        (strncmp(linein,"down",4)==0)) {
        changedir(linein);    
        goto mainloop;
    }

    if (strncmp(linein,"cd",2)==0) {        // restricted change directory
        if (my_uid < UIDRESTRICT) changedir(linein);
        else printf("not found: %s \n",linein);
        goto mainloop;
    }

    if (strcmp(linein,"up")==0) {       // move up one directory level
        chdirup(my_home);
        goto mainloop;
    }

    if (strcmp(linein,"home")==0) {     // go to the home (login) directory
        int err = chdir(my_home);
        if (!err)
            goto mainloop;
        else 
            printf("unable to change directories\n");
        goto mainloop;
    }

    if ((strncmp(linein,"newd ",5)==0) || (strncmp(linein,"created ",8)==0)) {    // create a directory
        createdir(linein);
        goto mainloop;
    }

    if ((strncmp(linein,"newf",4)==0) || (strncmp(linein,"createf",7)==0)) {   // create a new file, manually add contents
        createfile(linein);
        goto mainloop;
    }

    if (strncmp(linein,"tree ",5)==0) {     // show a tree from path
        tree(linein);
        goto mainloop;
    }

    if ((strncmp(linein,"del",3)==0) || (strncmp(linein,"delete",6)==0)) {    // delete either a file or directory
        delete(linein);
        goto mainloop;
    }

    if (strncmp(linein,"type",4)==0) {      // display a text file a page at a time
        type(linein);
        goto mainloop;
    }
   
    if (strncmp(linein,"copy",4)==0) {      // copy a file
        copy(linein);
        goto mainloop;
    }

    if (strncmp(linein,"rename",6)==0) {    // rename a file/directory
        renamefile(linein);
        goto mainloop;
    }

    if (strncmp(linein,"concat",6)==0) {    // concat file1 file2 into file1
        concat(linein);
        goto mainloop;
    }

    if (strncmp(linein,"filesort",4)==0) {  // sort a file
        filesort(linein);
        goto mainloop;
    }




    /* ------------------------- */
    /* ### External Commands ### */
    /* ------------------------- */

    if (strncmp(linein,"reload",6)==0) {    // reload the shell
        char **args = mpe_split_line(linein);
        if (args[1] == NULL) execv("/usr/local/bin/mpesh",NULL);
        execv(args[1],NULL);
        perror("reload ");
        goto mainloop;
    }


    if (strncmp(linein,"run",3)==0) {   // run a command in the current directory 
        char CMD[MAXLINE] = {}, TMP[MAXLINE]={};
        int pos=0;
        sscanf(linein,"%s %s",CMD,TMP);
        if (strlen(TMP)==0) {
            printf("usage: %s [command]\n",CMD);
            goto mainloop;
        }
        strcpy(CMD,"./");   // strip off the 'run', add the path
        strcat(CMD,TMP);    // add the local file
        /* now run the command */
        errno = 0;
        err = mpe_launch(mpe_split_line(CMD));
        if (!err)
            goto mainloop;
        printf("error");
        perror("cmd ");
        goto  mainloop;
    }

    if (strncmp(linein,"edit",4)==0) {     // run the predefined line editor on (optional) given filename
        if (testid())
            goto mainloop;
        int pos=0;
        char CMD[MAXLINE]={}, TMP[MAXLINE]={}, FILENAME[MAXLINE]={};
        sscanf(linein,"%s %s",CMD,TMP);
        strcpy(CMD,EDITOR);     // use predefined editor
        for (int n=0; n<strlen(TMP); n++)
            if ((isalnum(TMP[n]) || TMP[n]=='.'))   // avoid non-alpha chars xcpt .
                FILENAME[pos++] = TMP[n];
        sprintf(TMP,"%s %s",CMD,FILENAME);
        err = mpe_launch(mpe_split_line(TMP));
        if (!err)
            goto mainloop;
        printf("error");
        goto    mainloop;
    }

    if (strncmp(linein,"basic",5)==0) {     // run basic on (optional) given filename
        if (testid())
            goto mainloop;
        int pos=0;
        char CMD[MAXLINE]={}, TMP[MAXLINE]={}, FILENAME[MAXLINE]={};
        sscanf(linein,"%s %s",CMD,TMP);
        strcpy(CMD,BASIC);  // use predefined basic intrepretor
        for (int n=0; n<strlen(TMP); n++)
            if ((isalnum(TMP[n]) || TMP[n]=='.'))   // avoid non-alpha chars xcpt .
                FILENAME[pos++] = TMP[n];
        sprintf(TMP,"%s %s",CMD,FILENAME);
        err = mpe_launch(mpe_split_line(TMP));
        if (!err)
            goto mainloop;
        printf("error");
        goto    mainloop;
    }


    /* --- end of internal commands --- */



    /* Internal command not found, see if it's external */

    /* if uid < UIDRESTRICT run an external command */
    if (my_uid < UIDRESTRICT) {
        err = mpe_launch(mpe_split_line(linein));
        if (!err)
            goto mainloop;
        printf("error\n");
        goto mainloop;
    }



    /* ---- for uid's >= UIDRESTRICT ---- */

    /* ----------- end of command check ----------- */
    for (int n=0; n<MAXLINE; n++) {     // show 1st argument of string
        if (isalnum(linein[n]))
            printf("%c",linein[n]);
        else {
            printf(":");
            break;
        }
    }
    printf(" not found\n");
    goto mainloop;
}



/* ---------------------------------------------------- */
/* -------- start of functions and subroutines -------- */
/* ---------------------------------------------------- */


/* ----------- run an external command ------------- */


/* parse the linein into tokens */
#define MPE_TOK_DELIM " \t\r\n\a"
char **mpe_split_line(char *line) {

int bufsize = MAXLINE, position = 0;
char **tokens = malloc(bufsize * sizeof(char*));
char *token;

    if (!tokens) {
        fprintf(stdout, "token parse: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, MPE_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += MAXLINE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                fprintf(stdout, "memory error in splitline()\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, MPE_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}



/* actual command launch */
int mpe_launch(char **args) {
    pid_t pid, wpid;
    int status = 0;

    if (args[0] == NULL)
        return 1;

    pid = fork();
    if (pid < 0) {      // fork failed
        perror("external command error ");
        return 1;
    }

    if (pid == 0) {     // child process: exec the command w/args
        if (execvp(args[0], args) == -1) {
            perror(args[0]);
        }
        exit(EXIT_FAILURE);
    } 
    else {    // parent process
        do {        // wait until the child exits
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    if (WIFSIGNALED(status)){   // handle segfaults etc
        if (status == 11)
            printf("Error 11: Memory Error/Segfault\n");
        else
            if (my_uid >= UIDRESTRICT) 
            printf("\nSystem Error [%d] %s\n",status,strerror(status));
    }
    
    return 0;
}





/* ####################################################### */
/* ------------ internal commands and functions ---------- */
/* ####################################################### */


/* ######## QUIT ######### */
int quit(void) {    // exit the shell
    exit(0);
}



/* ####### CLS ######## */
void cls(void) {    // clear the screen
    for (int n=0; n<80; n++) 
        printf("\n");
    return;
}


/* ####### TESTID ######### */
int testid(void) {  // if uid >=UIDRESTRICT, return 1 else return 0
    if (my_uid < UIDRESTRICT) 
        return 0;
    else {
        puts(NOAUTH);
        return 1;
    }
}


/* convert given value to base 10. base is base of given value */
/* ex: conv 111000 2, conv 0xff00 16 */
void conv(char *line) {
    char **args = mpe_split_line(linein);
    if ((args[1] == NULL) || (args[2] == NULL)) {
        printf("usage: %s [value] [base]\n",args[0]);
        return;
    }
    errno = 0;
    long value = strtol(args[1],NULL,atoi(args[2]));
    if (errno != 0) { 
        perror("conv ");
        return;
    }
    else
        printf("%ld\n",value);
    return;
}



/* ######## DIR ######### */
void dir(char *linein) {    // simple directory listing, no options
#define MAXENT 4000     // max dirs and files per directory
#define MAXLEN 250      // max length of dir/file name
    DIR *dp;
    struct dirent *ep;
    struct stat buf;
    char CMD[MAXLINE]={}, DIRNAME[MAXLINE]={},FILENAME[MAXLINE]={}, DIRLIST[MAXENT][MAXLEN]={}, FILELIST[MAXENT][MAXLEN]={};
    sscanf(linein,"%s %s",CMD,DIRNAME);
    if (strlen(DIRNAME)==0) 
        strcpy(DIRNAME,getcwd(CMD,MAXLINE));
    errno = 0;
    dp = opendir(DIRNAME);
    if (dp == NULL) {
        perror("dir ");
        return;
    }
    /* security (no easy way to do) */
    printf("\nListing for %s\n\n",DIRNAME); 
    int dircnt = 0, filecnt = 0, col = 1;

    /* get listing */
    while ((ep = readdir(dp)) != NULL) {
        strncpy(FILENAME,ep->d_name,MAXLINE);
        if ((strcmp(FILENAME,".")==0) || (strcmp(FILENAME,"..")==0)) continue;  // don't show . & ..
        if (FILENAME[0]=='.') continue;     // don't show hidden files
        /* test dirs */
        if (ep->d_type == DT_DIR) {
            sprintf(DIRLIST[dircnt++],"%s",ep->d_name);
            continue;
        }
        /* test files */
        if (ep->d_type == DT_REG) { 
            stat(ep->d_name, &buf);        // put name in buf
            /* test executable */
            if ((buf.st_mode & S_IXUSR) && ((buf.st_mode & S_IFMT) == S_IFREG)){ // reg file, executable
                sprintf(FILELIST[filecnt++],"%s*",ep->d_name);  // executable
                continue;
            }
        }
        /* any other file type */
        sprintf(FILELIST[filecnt++],"%s",ep->d_name); // non-executable
        continue;
    }

    /* if nothing, show nothing */
    if ((filecnt == 0) && (dircnt == 0)) {
        printf("no files found\n");
        return;
    }

    /* sort the array */
    qsort(&FILELIST[0],filecnt,MAXLEN,cmpstringp);
    qsort(&DIRLIST[0],dircnt,MAXLEN,cmpstringp);

    /* print the array */
    printf("--- Files ---\n");
    for (int n=0; n<filecnt; n++) {
        if (strlen(FILELIST[n])==0) continue;
        printf("%-*s",20,FILELIST[n]);
        col++;
        if (strlen(FILELIST[n])>19){
            printf("\n");
            col=1;
        }
        if (col > 4) {
            printf("\n");
            col = 1;
        }
    }
    printf("\n");
    printf("--- Directories ---\n");
    for (int n=0; n<dircnt; n++) {
        if (strlen(DIRLIST[n])==0) continue;
        printf("%-*s",20,DIRLIST[n]);
        col++;
        if (col > 4) {
            printf("\n");
            col = 1;
        }
    }

    printf("\nFound %d files, %d directories\n",filecnt,dircnt);
    
    /* show available space on filesystem of this directory */
    struct statvfs dmbuf;
    fsblkcnt_t disk_avail;
    int err = statvfs(DIRNAME,&dmbuf);
    if (err==-1) perror("status ");
    disk_avail = 4 * dmbuf.f_bavail;   //NOTE: Not accurate on FAT filesystems
    printf("%ld MB free\n\n",disk_avail/1000);

    return;
}









/* ######## filesize ######## */
void filesize(char *linein) {   // get the size of a file 
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    struct stat buf;
    char *fname = &FILENAME[0];
    if (stat(fname, &buf) == -1) {
        printf("file %s not found\n",FILENAME);
        return;
    }
    printf("%s ",fname);
    printf("%ld bytes ",(long)buf.st_size);
    printf("(%lld blocks)\n",(long long) buf.st_blocks);
    return;
}



/* ##### readonly ###### */
void readonly(char *linein) {       // set filename to readonly
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    struct stat buf;
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    if (stat(FILENAME, &buf)==-1) {       // put name in buf
        perror("public ");
        return;
    }
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }
    /* if owner is r?x and public then change to r_x for all */
    if ((buf.st_mode & S_IXUSR) && (buf.st_mode & S_IXGRP)) { // exec & public
        chmod(FILENAME,S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR|S_IXGRP|S_IXOTH);
        return;
    }
    /* if owner is r?x and private the owner is r_x */
    if ((buf.st_mode & S_IXUSR) && (!(buf.st_mode & S_IXGRP))) {
        chmod(FILENAME,S_IRUSR|S_IXUSR);
        return;
    }
    /* if owner is r? and public then change to r and public */
    if (buf.st_mode & S_IRUSR) {
        chmod(FILENAME,S_IRUSR|S_IRGRP|S_IROTH);
        return;
    }

}



/* ### readwrite ### */
void readwrite(char *linein) {      // set filename to read write
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    struct stat buf;
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    if (stat(FILENAME, &buf)==-1) {       // put name in buf
        perror("public ");
        return;
    }
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }
    /* if owner is r_x and grp is r then change owner to rwx grp to r_x r_x */
    if((buf.st_mode & S_IXUSR) && (buf.st_mode & S_IXGRP)) {
        chmod(FILENAME,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        return;
    }
    /* if owner is r_x and grp is not r then owner is rwx */
    if ((buf.st_mode & S_IXUSR) && (!(buf.st_mode & S_IXGRP))) {
        chmod(FILENAME,S_IRWXU);
        return;
    }
    /* if owner is r__ and grp is r__ then owner RW_ and grp oth is R__ */
    if (buf.st_mode & S_IRGRP) {
        chmod(FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        return;
    }

    puts("no change");
    return;
}

/* ### public ### */
void public(char *linein) {     // set file to public r/w
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    struct stat buf;
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    if (stat(FILENAME, &buf)==-1) {       // put name in buf
        perror("public ");
        return;
    }
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }
    /* if owner is rwx, set group and other to r_x */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR) && (buf.st_mode & S_IXUSR)) {   // owner: rwx set
        chmod(FILENAME,S_IRWXU|S_IRGRP|S_IROTH|S_IXGRP|S_IXOTH);    // set rwx for own, rx for rest
        return;
    }
    /* if owner is rw, set grp and oth to r also */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR)) { // only rw set
        chmod(FILENAME,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);    // set r__ for all
        return;
    }
    /* if owner is r_x only, set all to r_x */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IXUSR)) {
        chmod (FILENAME,S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR|S_IXGRP|S_IXOTH);
        return;
    }
    /* if owner is r only, set grp and oth to r also */
    if (buf.st_mode & S_IRUSR) { // only read set
        chmod(FILENAME,S_IRUSR|S_IRGRP|S_IROTH);    // set read for all
        return;
    }

    puts("no change");
    return;
}

/* ### private ### */
void private(char *linein) {    // set file to rw only for owner, null for group & other 
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    struct stat buf;
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    if (stat(FILENAME, &buf)==-1) {       // put name in buf
        perror("private ");
        return;
    }
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }
    /* if owner is rwx, set group and other to null */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR) && (buf.st_mode & S_IXUSR)) {   // owner: rwx set
        chmod(FILENAME,S_IRWXU);    // set rwx for own, rx for rest
        return;
    }
    /* if owner is rw, set grp and oth to null */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR)) { // only rw set
        chmod(FILENAME,S_IRUSR|S_IWUSR);    // set r__ for all
        return;
    }
    /* if owner is r_x only, set grp and oth to null */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IXUSR)) {
        chmod (FILENAME,S_IRUSR|S_IXUSR);
        return;
    }
    /* if owner is r only, set grp and oth to null */
    if (buf.st_mode & S_IRUSR) { // only read set
        chmod(FILENAME,S_IRUSR);    // set read for all
        return;
    }

    puts("no change");
    return;
}



/* chdir/cd/down - jump to directory [path] */
void changedir(char *linein) {
    char CMD[MAXLINE]={}, DIRNAME[MAXLINE]={}, tmp[MAXLINE]={};
    int err=0, pos=0;
    sscanf(linein,"%s %s",CMD,DIRNAME);
    if (strlen(DIRNAME)==0) {
        printf("usage: %s [path]\n",CMD);
        return;
    }
    if ((strcmp(CMD,"down")==0) || (my_uid >= UIDRESTRICT)) {
        for (int n=0; n<strlen(DIRNAME); n++) 
            if (isalnum(DIRNAME[n]))
                tmp[pos++] = DIRNAME[n];    // strip off all but alnum chars
        strcpy(DIRNAME,tmp);
    }
    err = chdir(DIRNAME);
    if (!err) return;
    else
        perror("chdir ");
    return;
}




/* up - go up one directory level */
void chdirup(char *linein) {
    char tmp[MAXLINE]={};
    errno = 0;
    getcwd(tmp,MAXLINE-1);
    if ((my_uid >= UIDRESTRICT) && (strcmp(linein,tmp)==0)) return; // limited for restricted users
    if (!chdir("../")) return;
    else
        perror("up ");
    return;
}





/* create - create a directory */
void createdir(char *linein) {
    char CMD[MAXLINE]={}, DIRNAME[MAXLINE]={}, tmp[MAXLINE]={};
    int pos = 0, err = 0;
    sscanf(linein,"%s %s",CMD, DIRNAME);
    //printf("%s\n",DIRNAME);
    if (strlen(DIRNAME)==0) {
        printf("usage: %s [directory name]\n",CMD);
        return;
    }
    for (int n=0; n<strlen(DIRNAME); n++) 
        if (isalnum(DIRNAME[n]))
            tmp[pos++] = DIRNAME[n];
    err = mkdir(tmp,0777);
    if (!err) 
        return;
    else
        perror("create ");
    return;
}




/* new - create a new file */
void createfile(char *linein) {
    FILE *infile;
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={}, tmp[MAXLINE]={}, line[MAXLINE]={};
    int pos = 0;
    sscanf(linein,"%s %s",CMD, FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    infile = fopen(FILENAME,"r");
    if (infile != NULL) {
        printf("file exists: %s\n",FILENAME);
        fclose(infile);
        return;
    }
    infile = fopen(FILENAME,"w");
    if (infile == NULL) {
        printf("could not create file: %s\n",FILENAME);
        return;
    }
    printf("file created: %s\n",FILENAME);
    printf("Enter text: '.end' on an empty line saves and exits\n");
    // not even a line editor, but enter a line of text until '.end' is entered on an empty line 
    while (1) {
        fgets(line,sizeof(line)-1,stdin);
        if (strncmp(line,".end",4)==0) break;
        fprintf(infile,"%s",line);
    }
    
    fclose(infile);
    return;
}





/* ### COPY ### */
void copy(char *linein) {   // copy oldfile -> newfile
    char ch;
    char line[MAXLINE] = {};
    char CMD[MAXLINE]={}, OLDFILENAME[MAXLINE]={}, NEWFILENAME[MAXLINE] = {}, TMP[MAXLINE]={};
    int pos = 0, err = 0;
    struct stat buf;
    sscanf(linein,"%s %s %s",CMD, OLDFILENAME, NEWFILENAME);
    if ((strlen(OLDFILENAME)==0) || (strlen(NEWFILENAME)==0)) {
        printf("usage: %s [oldfilename] [newfilename]\n",CMD);
        return;
    }
    
    /* open the file */
    char    c[1024]; // or any other constant you like
    FILE *infile = fopen(OLDFILENAME, "r");
    if (infile == NULL) {
        printf("cannot open file %s for read\n",OLDFILENAME);
        return;
    }
    FILE *outfile = fopen(NEWFILENAME, "r");   //test if file exists
    if (outfile != NULL) {  // file exists
        fclose(outfile);
        printf("warning: file %s already exists\n",NEWFILENAME);
        return;
    }
    outfile = fopen(NEWFILENAME,"w");       // open to write to
    if (outfile == NULL) {
        printf("cannot open %s to writing\n",NEWFILENAME);
        return;
    }
    
    while (!feof(infile)) {
        size_t bytes = fread(c, 1, sizeof(c), infile);
        if (bytes) {
            fwrite(c, 1, bytes, outfile);
        }
    }

    //close streams
    fclose(infile);
    fflush(outfile);
    fclose(outfile);
    
    /* if OLDFILENAME is executable, make sure NEWFILENAME is too */
    lstat(OLDFILENAME, &buf);
    if (((buf.st_mode & S_IXUSR) != 0) && ((buf.st_mode & S_IFMT) == S_IFREG))
        chmod(NEWFILENAME,S_IRWXU);


    return;
}




/* ####### CONCAT ########## */
void concat(char *linein) {     // concat file2 into file1
    char CMD[MAXLINE]={}, FILENAME1[MAXLINE]={}, FILENAME2[MAXLINE] = {};

    sscanf(linein,"%s %s %s",CMD, FILENAME1, FILENAME2);
    if ((strlen(FILENAME1)==0) || (strlen(FILENAME1)==0)) {
        printf("usage: %s [filename1] [filename2]\n",CMD);
        return;
    }
    FILE *file1 = fopen(FILENAME1,"a");
    if (file1 == NULL) {
        printf("cannot open %s for append\n",FILENAME1);
        return;
    }
    FILE *file2 = fopen(FILENAME2,"r");
    if (file2 == NULL) {
        printf("cannot open %s for read\n",FILENAME2);
        fclose(file1);
        return;
    }
    char c[1024];
    while (!feof(file2)) {
        size_t bytes = fread(c,1,sizeof(c), file2);
        if (bytes)  {
            fwrite(c,1,bytes,file1);
        }
    }

    fflush(file1);
    fclose(file1);
    fclose(file2);

    return;
}




/* ####### FILESORT ########## */
void filesort(char *linein) {       // sort a file, save to same file
    struct stat buf;
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={};
    sscanf(linein,"%s %s",CMD,FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    /* test for ownership here because we re-write the file */
    stat(FILENAME, &buf);
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }
    
    
    /* read in the file - find the length and longest element */
    int filelen = 0, filewidth = 0;
    char st[1024];
    FILE *infile = fopen(FILENAME,"r");
    while (!feof(infile)) {
        fgets(st,sizeof(st),infile);
        if (strlen(st) > filewidth)
            filewidth = strlen(st)+1;
        filelen++;
    }
    fclose(infile);
    //printf("line count = %d   width = %d\n",filelen,filewidth);
    /* load the file into an array */
    char buffer[filelen][filewidth];
    infile = fopen(FILENAME,"r");
    if (infile == NULL) {
        printf("cannot reopen file %s\n",FILENAME);
        return;
    }
    filelen = 0;
    while (!feof(infile)) 
            fgets(buffer[filelen++],filewidth,infile);
    /* we have file contents */
    fclose(infile);
    /* sort it */
    qsort(&buffer[0],filelen,filewidth,cmpstringp);
    /* re-write the file */
    infile = fopen(FILENAME,"w");
    if (infile == NULL) {
        printf("cannot write to file %s\n",FILENAME);
        return;
    }
    for (int n=0; n<filelen; n++)
        fprintf(infile,"%s",buffer[n]);
    fflush(infile);
    fclose(infile);
    /* done */
    return;
}



/* ### TYPE ### - display a text file */
void type(char *linein) {
    FILE *infile;
    char ch;
    char line[MAXLINE] = {};
    char CMD[MAXLINE]={}, FILENAME[MAXLINE]={}, TMP[MAXLINE]={};
    int pos = 0, err = 0;
    sscanf(linein,"%s %s",CMD, FILENAME);
    if (strlen(FILENAME)==0) {
        printf("usage: %s [filename]\n",CMD);
        return;
    }
    /* test security */
    if (my_uid >= UIDRESTRICT) {
        if ((FILENAME[0]=='.') || (FILENAME[0]=='/')) {
            puts(NOAUTH);
            return;
        }
    }
    /* open the file */
    infile = fopen(FILENAME,"r");
    if (infile == NULL) {
        printf("cannot open file %s\n",FILENAME);
        return;
    }
    int row = 0;
    while (1) {
        fgets(line,MAXLINE,infile);
        if (feof(infile))
            break;
        printf("%s",line);
        row++;
        if (row <24) continue;
        /* pause */
        printf("\n--- MORE ---");
        ch = fgetc(stdin);
        if (ch == 'q') {
            ch = fgetc(stdin);  // get terminating cr
            fclose(infile);
            printf("\n");
            return;
        }
        row = 0;
        continue;
    }
    fclose(infile);
    printf("\n");
    return;
}







/* rename - change the name of a file */
void renamefile(char *linein) {
    struct stat buf;
    int err=0;
    char CMD[MAXLINE]={}, OLDFILENAME[MAXLINE]={}, NEWFILENAME[MAXLINE]={};
    sscanf(linein,"%s %s %s",CMD,OLDFILENAME,NEWFILENAME);
    if ((strlen(OLDFILENAME)==0) || (strlen(NEWFILENAME)==0)){
        printf("usage: %s [oldfilename] [newfilename]\n",CMD);
        return;
    }
    /* test for ownership here */
    stat(OLDFILENAME, &buf);
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }

    err = rename(OLDFILENAME,NEWFILENAME);
    if (err == 0)
        return;
    else
        perror("rename ");
    return;
}








/* delete - either a directory (empty) or a file */
void delete(char *linein) {
    struct stat buf;
    char CMD[MAXLINE]={}, DIRNAME[MAXLINE]={}, tmp[MAXLINE]={};
    int pos = 0, err = 0;
    sscanf(linein,"%s %s",CMD, DIRNAME);
    
    if (strlen(DIRNAME)==0) {
        printf("usage: %s [directory name]\n",CMD);
        return;
    }
    stat(DIRNAME, &buf);
    if (buf.st_uid != my_uid) {
        printf("Not Owner\n");
        return;
    }

    /* DIRNAME is the name of the file/dir to delete */

    if (is_regular_file(DIRNAME)==1) {    // regular file to delete 
        err = unlink(DIRNAME);
        if (!err) 
            return;
        else
            perror("delete ");
        return;
    }

    if (isDirectory(DIRNAME)) {     // delete directory
        err = rmdir(DIRNAME);
        if (!err)
            return;
        else
            perror("delete ");
        return;
    }
    printf("unknown filetype - operation canceled\n");
    return;
}


/* show system/user status */
void status(char *linein) {
    char **args = mpe_split_line(linein);
    struct statvfs buf;
    char disk[50];
    fsblkcnt_t disk_avail;
    printf("\nUSER INFORMATION\n");
    printf("Login Info (name/ID) %s / %d\n",my_name,my_uid);
    printf("Home Directory: %s\n",my_home);
    printf("\nDISKS\n");
    int err = statvfs("/user/home/pi/MPE/MPE/mpesh",&buf);
    if (err) perror("status ");
    disk_avail = 4 * buf.f_bavail;
    printf("avail: %ld MB\n",disk_avail/1000);


    return;
}



/* return 1 if filename is a regular file */
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}



/* return 1 if filename is a directory */
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
