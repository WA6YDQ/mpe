/* file.c  k theis 7/2020
 *
 * file [filename] - create/populate a new file 
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mpe.h"

int main(int argc, char**argv) {

    int ct = 1;
    //char linein[MAXLINE];
    FILE *outfile;

    if (argc < 2) {
        puts("usage: file [filename]");
        return 0;
    }

    /* create all files listed in command line */
    while (argc-- > 1) {

        /* see if it already exists */
        if (!access(argv[ct],F_OK)) {
            printf("file %s exists\n",argv[ct]);
            return 1;
        }

        /* create an empty file */
        outfile = fopen(argv[ct],"w");
        if (!outfile) {
            printf("cannot create %s\n",argv[ct]);
            return 1;
        }   
    
        fflush(outfile);
        fclose(outfile);

        ct++;
    }

    /* fin */
    return 0;
}

