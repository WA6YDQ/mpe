/*
 * dir.c - simple dir lister
 * (see ldir.c for long format)
 *
 * k theis 7/2020
 *
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "mpe.h"
#include <dirent.h>
#include <libgen.h>

void printEntry(char *);
int colorEntry(char *);
int filecnt=0, dircnt=0;
int row = 0;

int main(int argc, char **argv) {
    char fname[MAXLINE], dname[MAXLINE];
    struct dirent **namelist;
    struct stat buf;
    int n, total_size = 0;

    if ((argc == 1) || (argc == 2)) {    /* no shell supplied args */
        if (argc == 1) 
            strcpy(dname,".");
        if (argc == 2) 
            strcpy(dname,argv[1]);
        lstat(dname,&buf);
        if (!(S_ISDIR(buf.st_mode))) {     // 2nd arg is file or dir/file */
            colorEntry(dname);
            printEntry(dname);
            printf("\n");
            white();
            return 0;
        }
        if (S_ISDIR(buf.st_mode))       // 2nd arg is dir name
            strcat(dname,"/");
        
        n=scandir(dname,&namelist,NULL,alphasort);  // get all files in dir

        if (n<0) perror("");
        for (int cnt=0; cnt < n; cnt++) {   // show all files in dir
            if ((strcmp(namelist[cnt] -> d_name,".")==0) || 
                (strcmp(namelist[cnt] -> d_name,"..")==0)) {
                free(namelist[cnt]);
                continue;
            }

            memset(fname,0,MAXLINE);
            strcpy(fname,dname);            // prepend dirname for later lstat
            strcat(fname,namelist[cnt] -> d_name);
            total_size += colorEntry(fname);
            printEntry(fname);
            free(namelist[cnt]);
        }
        goto summarize;
    }


    /* argc > 2 : the shell supplied the file list w/the dirname prepended */
    
    argv++; argc--; // skip argv[0]

    while (argc--) {
        colorEntry(*argv);       // set color based on file type
        printEntry(*argv);      // display it
        argv++;
    }
   
summarize:
    /* summarize */
    white();
    printf("\n");
    if ((filecnt == 0) && (dircnt == 0)) 
        puts("no files found");
    else {
        printf("found %d files and %d directories\n",filecnt,dircnt);
        printf("%d bytes used\n",total_size);
    }
    return 0;
}


/* print file name */
void printEntry(char *entry) {

        /* show the file/dir */
        if ((row >= 3) && (strlen(basename(entry)) > 20)){
            printf("\n");
            row = 0;
        }
        printf("%-*s",20,basename(entry));

        /* don't let dir names touch */
        if (strlen(basename(entry)) >=  19) {
            int val = strlen(basename(entry));
            while (val>=20) {
                val = (val-20)%20;
                printf("%-*s",20-val,"\0");
                row += 1;
            }
        }

        row += 1;
        if (row >= 4)  {
            printf("\n");
            row = 0;
        }
        return;
}



/* set text color based on file type */
int colorEntry(char *ent) {
struct stat buf;
int filesize = 0;

    /* get file information */
        lstat(ent, &buf);

        /* directory */
        if (S_ISDIR(buf.st_mode)) {
            blue();
            dircnt++;
        }

        /* regular file set executable*/
        if ((S_ISREG(buf.st_mode)) && (buf.st_mode & S_IXUSR)) {
            yellow();
            filecnt++;
            filesize = buf.st_size;
        }

        /* regular file, not executable */
        if ((S_ISREG(buf.st_mode)) && (!(buf.st_mode & S_IXUSR))) {
            white();
            filecnt++;
            filesize = buf.st_size;
        }

        /* block special, chr special, fifo */
        if (S_ISCHR(buf.st_mode) || S_ISBLK(buf.st_mode) || S_ISFIFO(buf.st_mode)) {
            magenta();
            filecnt++;
        }

        /* sym link */
        if (S_ISLNK(buf.st_mode)) {
            cyanBold();
            filecnt++;
        }

        return filesize;
}
