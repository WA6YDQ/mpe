/* help.c  display help files for mpe os
 *
 * k theis 7/2020
 *
 */

#define HELPFILE  "/helpfiles/"
#define TOKENBUF 10000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <wordexp.h>
#include <unistd.h>
#include <libgen.h>
#include "mpe.h"

int fatal (char *error) {
    printf("fatal error: %s\n",error);
    exit(1);
}

int main(int argc, char **argv) {

wordexp_t p;        // for wordexp
char **w;           // for wordexp
char **args;        // for wordexp
int i = 0, err = 0; // gen purpose
FILE *infile;

    if (argc == 1) {    // show all help files
        char line[MAXLINE];
        char fname[MAXLINE];
        //char *pos;
        int row=1;
        strcpy(line,"*.hlp");
        chdir(HELPFILE);

        /* get list of help files */
        err = wordexp(line, &p, 0); 
        if (err != 0) {
            puts("parse error");
            exit(0);
        }
        
        /* convert to tokens */
        w = p.we_wordv;
        args = malloc(TOKENBUF);
        if (!args) fatal("memory error");
        for (i=0; i<p.we_wordc; i++)
           args[i] = w[i];
        args[i] = (char *)NULL;

        /* show list */
        green();
        puts("Help is available for:");
        for (i=0; i<p.we_wordc; i++) {
            memset(fname,0,MAXLINE-1);
            strncpy(fname,args[i],strlen(args[i])-4);
            printf("%-*s",20,fname);
            row++;
            if (row > 4) {
                row = 1;
                printf("\n");
            }
        }

        free(args);
        printf("\n");
        white();
        exit(0);
    }


    if (argc == 2) {    // display help file 
        char line[MAXLINE];
        char fname[MAXLINE];
        strcpy(fname,HELPFILE);
        strcat(fname,argv[1]);
        strcat(fname,".hlp");

        if (access(fname,F_OK)) {
            green();
            printf("help not found for %s\n",argv[1]);
            white();
            exit(0);
        }

        infile = fopen(fname,"r");
        if (!infile) fatal("error opening help file");
        printf("\n");

        /* display the file */
        green();
        while (1) {
            fgets(line,MAXLINE-1,infile);
            if (feof(infile)) break;
            green();
            printf("%s",line);
        }
        white();
        fclose(infile);
    }

    if (argc > 2) {
        green();
        puts("usage: help [command]");
        white();
    }
    exit(0);
}

