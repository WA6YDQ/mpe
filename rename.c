/* rename.c   k theis 7/2020
 *
 * rename oldfile newfile
 *
 *  Note: this uses hard links, so hard-link's across 
 *  devices doesn't work. As of 7/2020, mpe doesn't use
 *  multiple storage devices, so that's not a problem.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "mpe.h"



int main(int argc, char **argv) {
    int err;
    //char linein[MAXLINE] = {0x0};
    struct stat buf;

    if (argc < 3) {
        puts("usage: rename [old path/name] [new path/name]");
        return 0;
    }

    /* test for existance of source */
    if (access(argv[1],F_OK) == -1) {
        printf("%s not found\n",argv[1]);
        return 1;
    }
   
    /* test if read only */
    stat(argv[1],&buf);
    if (!(buf.st_mode & S_IWUSR)) {
        printf("%s is read only\n",argv[1]);
        return 1;
    }


    if (!access(argv[2],F_OK)) {
        printf("file %s exists\n",argv[2]);
        return 1;
    }

    err = link(argv[1],argv[2]);
    if (err) {
        perror("");
        return 1;
    }

    unlink(argv[1]);
    return 0;
}



