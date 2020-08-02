/*  status.c - show system status
 *
 *  k theis 7 2020
 *
 */  

#define TOKENBUF 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <wordexp.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "mpe.h"

int fatal(char *error) {
    printf("fatal error: %s\n",error);
    exit(1);
}

int main(void) {
FILE *infile;
char line[MAXLINE];
char mounts[100][MAXLINE];
char **args;
char **w;
int i=0, err=0;
wordexp_t p;
struct statvfs buf;
struct statfs dbuf;
fsblkcnt_t disk_avail;
fsblkcnt_t f_bfree;
char tmp[80];

	return 0;		// currently this command does nothing

    green();

    /* show # users logged in */
    int usernum = 0;
    memset(line,0,sizeof(line));
    infile = fopen("/tmp/mpe_login_counter","r");
    if (infile) {
        fgets(line,20,infile);
        usernum = atoi(line);
        printf("\nNumber of logged in users: %d\n",usernum);
        printf("\n");
        fclose(infile);
        infile = NULL;
    }
    else
        printf("Unable to read count of active users.\n");
   


    /* show mounted filesystems */
    int mfs = 0;
    infile = fopen("/proc/mounts","r");
    if (infile) {
        printf("Mounted Filesystems\n");
        /* get a list of mounted filesystems */
        while (1) {
            memset(line,0,80);
            fgets(line,40,infile);
            line[strlen(line)-1] = '\0';
            if (feof(infile)) break;
            if (strncmp(line,"/dev/root",8)==0) {
                strcpy(mounts[mfs],line);
                mfs += 1;
                continue;
            }

            if (strncmp(line,"/dev/s",6)==0) {
                strcpy(mounts[mfs],line);
                mfs += 1;
                continue;
            }

        }
        /* tokenize results, just get device and mount point */
        for (int i=0; i<mfs; i++) {
            int ct = 0;
            err = wordexp(mounts[i],&p,0);
            if (err != 0) {
                puts("token error");
                continue;
            }
            w = p.we_wordv;
            //printf("counts %d\n",p.we_wordc);

            args = malloc(TOKENBUF);
            if (!args) fatal("memory error");
            for (ct=0; ct<p.we_wordc; ct++)
                args[ct] = w[ct];
            args[ct] = (char *)NULL;
            printf("%-*s %-*s \n",12,args[0],20,args[1]);
            free(args);
        }
    }
    else
        puts("Unable to find active filesystems");



    /* get current user stats */
    printf("\nUser Statistics\n");
    
    struct passwd *pw;
    uid_t uid, euid;
    uid = getuid();
    euid = geteuid();
    pw = getpwuid(uid);
    if (pw) {
        printf("My User ID: %zd \n",uid);
        printf("Effective User ID: %zd \n",euid);
        printf("Login Name: %s\n",pw->pw_name);
    }

    printf("\n");

    white();
    exit(0);
}

