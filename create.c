/* newd.c  k theis 7/2020
 *
 * newd [pathname] - create a new directory
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mpe.h"


int main(int argc, char**argv) {
    int err = 0;
    //char linein[MAXLINE];
    //FILE *outfile;

    if (argc != 2) {
        puts("usage: newd [pathname]");
        return 0;
    }

    /* see if it already exists */
    if (!access(argv[1],F_OK)) {
        printf("directory %s exists\n",argv[1]);
        return 1;
    }

    /* create directory */
    err = mkdir(argv[1],S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    if (!err) return 0;
    perror("");
    return 1;
}
