/*  readwrite.c  k theis 7/2020
 *
 *  readwrite [filename] - set perms to read/write
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
        puts("usage: readwrite [filename]");
        return 0;
    }

	while (argc-- > 1) {

    /* check if file exists */
    if (stat(argv[cnt], &buf) == -1) {
        perror("");
		cnt++;
        continue;
    }

    /* check ownership */
    if (buf.st_uid != getuid()) {
        puts("not owner");
		cnt++;
        continue;
    }

    /* if owner is r_x and grp is r then change owner to rwx grp to r_x r_x */
    if((buf.st_mode & S_IXUSR) && (buf.st_mode & S_IXGRP)) {
        chmod(argv[cnt],S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
        cnt++;
		continue;
    }

    /* if owner is r_x and grp is not r then owner is rwx */
    if ((buf.st_mode & S_IXUSR) && (!(buf.st_mode & S_IXGRP))) {
        chmod(argv[cnt],S_IRWXU);
        cnt++;
		continue;
    }

    /* if owner is r__ and grp is r__ then owner RW_ and grp oth is R__ */
    if (buf.st_mode & S_IRGRP) {
        chmod(argv[cnt],S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        cnt++;
		continue;
    }

    printf("no change to %s\n",argv[cnt++]);
	continue;
	}

    return 0;

}

