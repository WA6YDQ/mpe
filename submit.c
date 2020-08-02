/* submit.c
 * batch processor for mpe
 *
 * k theis 7/2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "mpe.h"

int quit(void) {
	exit(0);
}

int main (int argc, char **argv) {
	char cmds[100][40] = {};		// hold commands (max 100)
	char ch[MAXLINE+1];
	char BUF[MAXLINE] = {};
	char* p;
	char* tok;
	char** args;
	int cnt = 0;
	int err = 0;
	pid_t pid, wpid;
	int status = 0;
	int i = 0;
	FILE *infile, *errfile;

	if (argc == 1) return 0;
	infile = fopen(argv[1],"r");
	if (infile == NULL) return 1;

	/* open error file */
	strcpy(BUF,argv[1]);
	strcat(BUF,".error");
	errfile = fopen(BUF,"w");
	if (errfile == NULL) return 1;

	/* main loop */
	while (1) {
		cnt = 0;
		err = 0;

		/* clear buffers */
		memset(ch,0,MAXLINE);
		memset(cmds,0,4000);
		
		/* get commands from file */
		fgets(ch,MAXLINE,infile);
		if (feof(infile)) {		// file end
			fclose(infile);
			fclose(errfile);
			exit(0);
		}
		if (ch[0] == '\n') continue;
		ch[(strlen(ch)-1)] = '\0';  // remove trailing newline

		/* split line to tokens */
		tok = ch;
		p = strtok(tok," ");
		strcpy(cmds[cnt++],p);		
		while(1) {
			tok = NULL;
			p = strtok(tok," \t\r\n");
			if (!p) break;
			strcpy(cmds[cnt++],p);
		}

		args = malloc(4000);
		if (!args) {
			puts("memory error");
			continue;
		}

		/* put strings into pointer array for execvp */
		for (i=0; i<cnt; i++) { 
			args[i] = cmds[i];
		}
		args[i] = (char *)NULL;

		/* test for local commands */
		if (strcmp(args[0],"exit")==0) {
			fclose(infile);
			fclose(errfile);
			return 0;
		}

		/* ignore comments */
		if (args[0][0] == '#') continue;

		/* execute command */
		pid = fork();
		if (pid < 0) {	// failed
			perror("bad fork");
			free(args);
			continue;
		}
		if (pid == 0) {	// this is child
			if (execvp(args[0],args) == -1) {// -1 = exec failed
				perror(args[0]);
			}
			exit(EXIT_FAILURE);		// exit child
		} else {
			// parent
			do {	// wait for child 
				wpid = waitpid(pid, &status, WUNTRACED);
			} while (!WIFEXITED(status) && !WIFSIGNALED(status));
		}

		/* test return status */
		err = WEXITSTATUS(status);
		if (err)
			fprintf(errfile,"%s : command returned error\n",args[0]);
		//if (WIFEXITED(status)) 	// normal
		//	puts("exited normally");
		if (WIFSTOPPED(status)) 
			printf("stopped: %d\n", WSTOPSIG(status));
		if (WIFSIGNALED(status)) 		// segfault etc
			printf("killed: %s\n",WTERMSIG(status));

			
		/* and do next */
		free(args);
		printf("\n");
		continue;

	}



	return 0;
}


