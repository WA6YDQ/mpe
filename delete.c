/*  delete.c  k theis 7/2020
 *
 *  delete [path] - delete a file or (empty) directory
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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



int main(int argc, char **argv) {
int err = 0;
struct stat buf;
uid_t uid;
int ct = 1;

    if (argc < 2) {
        puts("usage: delete [path]");
        return 0;
    }

    while (argc-- > 1) {

        /* test for existance */
        if (access(argv[ct], F_OK) == -1) {
            printf("%s not found\n",argv[ct++]);
            continue;
        }

        /* see if we own it */
        stat(argv[ct], &buf);
        if ((buf.st_uid) != geteuid()) { 
            printf("%s not owner\n",argv[ct++]);
            continue;
        }

        /* test if read only */
        if (!(buf.st_mode & S_IWUSR)) {
            printf("%s is read only\n",argv[ct++]);
            continue;
        }

        if (is_regular_file(argv[ct])==1) {    // regular file to delete 
            err = unlink(argv[ct++]);
            if (!err)
                continue;
            else
                perror("");
            continue;
        }

        if (isDirectory(argv[ct])) {     // delete directory
            err = rmdir(argv[ct++]);
            if (!err)
                continue;
            else
                perror("");
            continue;
        }
    
        printf("unknown filetype: %s\n",argv[ct++]);
        continue;
    }
}

