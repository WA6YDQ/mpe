/*  readonly.c   k theis 7/2020
 *
 *  readonly [path] - change file to readonly
 *
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char **argv) {
struct stat buf;
int cnt=1;

    if (argc == 1) {
        puts("usage: readonly [file]");
        return 0;
    }

	while (argc-- > 1) {

    /* test file existance */
    if (stat(argv[cnt], &buf) == -1) {
        perror("");
		cnt++;
        continue;
    }

    /* test owner */
    if (buf.st_uid != getuid()) {
        puts("not owner");
		cnt++;
        continue;
    }

    /* if owner is r?x and public then change to r_x for all */
    if ((buf.st_mode & S_IXUSR) && (buf.st_mode & S_IXGRP)) { // exec & public
        chmod(argv[cnt],S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR|S_IXGRP|S_IXOTH);
        cnt++;
		continue;
    }

    /* if owner is r?x and private the owner is r_x */
    if ((buf.st_mode & S_IXUSR) && (!(buf.st_mode & S_IXGRP))) {
        chmod(argv[cnt],S_IRUSR|S_IXUSR);
        cnt++;
		continue;
    }
    
    /* if owner is r? and public then change to r and public */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IROTH)) {
        chmod(argv[cnt],S_IRUSR|S_IRGRP|S_IROTH);
        cnt++;
		continue;
    }

	/* change owner to r-- */
	chmod(argv[cnt++],S_IRUSR);
	continue;


    printf("no change to %s\n",argv[cnt]);
	cnt++;
	continue;

	}

    return 0;
}
