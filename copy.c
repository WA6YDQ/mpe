/* copy.c  k theis 7/2020
 *
 * copy oldfile newfile
 *
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "mpe.h"

int main(int argc, char **argv) {
FILE *infile, *outfile;
struct stat buf;
char c[1024];       /* read buffer */
char linein[MAXLINE];

    if (argc < 3) {
        puts("usage: copy [oldfile] [newfile]");
        return 0;
    }

    /* test if newfile exists */
    infile = fopen(argv[2],"r");
    if (infile) {
        printf("file %s exists - overwrite (y/n)?: ",argv[2]);
        fgets(linein,MAXLINE,stdin);
        if (!((linein[0] == 'y') || (linein[0] == 'Y'))) {
            puts("aborted");
            return 1;
        }
        fclose(infile);
    }

    /* look for oldfile */
    infile = fopen(argv[1],"r");
    if (!infile) {
        printf("%s not found\n",argv[1]);
        return 1;
    }

    /* create newfile */
    outfile = fopen(argv[2],"w");
    if (!outfile) {
        printf("cannot create %s\n",argv[2]);
        fclose(infile);
        return 1;
    }

    /* so far, so good */
    while (!feof(infile)) {
        size_t bytes = fread(c, 1, sizeof(c), infile);
        if (bytes) 
            fwrite(c, 1, bytes, outfile);
    }

    /* close streams */
    fclose(infile);
    fflush(outfile);
    fclose(outfile);

    /* if oldfile is executable, make sure newfile is also */
    stat(argv[1], &buf);
        fflush(stdout);
    if ((buf.st_mode & S_IXUSR) && (buf.st_mode & S_IFREG))
        chmod(argv[2],S_IRWXU|S_IXGRP|S_IXOTH|S_IRGRP|S_IROTH);

    printf("copied %lu bytes\n",buf.st_size);


    /* fin */
    return 0;
}

    
