/*  public.c  k theis 7/2020
 *
 *  public [filename] - set perms to public
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
int cnt = 1;

    if (argc == 1) {
        puts("usage: public [filename]");
        return 0;
    }

	while (argc-- > 1) {

    /* test file existance */
    if (stat(argv[cnt], &buf) == -1) {
        perror("");
        cnt++;
		continue;
    }

    /* test ownership */
    if (buf.st_uid != getuid()) {
        puts("not owner");
		cnt++;
        continue;
    }

    /* if owner is rwx, set group and other to r_x */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR) && (buf.st_mode & S_IXUSR)) {   // owner: rwx set
        chmod(argv[cnt],S_IRWXU|S_IRGRP|S_IROTH|S_IXGRP|S_IXOTH);    // set rwx for own, rx for rest
        cnt++;
		continue;
    }

    /* if owner is rw, set grp and oth to r also */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IWUSR)) { // only rw set
        chmod(argv[cnt],S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);    // set r__ for all
        cnt++;
		continue;
    }

    /* if owner is r_x only, set all to r_x */
    if ((buf.st_mode & S_IRUSR) && (buf.st_mode & S_IXUSR)) {
        chmod (argv[cnt],S_IRUSR|S_IRGRP|S_IROTH|S_IXUSR|S_IXGRP|S_IXOTH);
        cnt++;
		continue;
    }

    /* if owner is r only, set grp and oth to r also */
    if (buf.st_mode & S_IRUSR) { // only read set
        chmod(argv[cnt],S_IRUSR|S_IRGRP|S_IROTH);    // set read for all
        cnt++;
		continue;
    }

    printf("no change to %s\n",argv[cnt]);

	cnt++;
	continue;
	}

    return 0;
}


