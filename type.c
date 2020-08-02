/*  type.c  k theis 7/2020
 *
 *  type [filename]
 *
 *  show contents by line of ascii filename 
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpe.h"

int main(int argc, char **argv) {
int row = 0, filenum = 1;
char line[MAXLINE-1] = {0x0};
char ch;
FILE *infile;

    if (argc == 1) {
        puts("usage: type file [file] [file] ...");
        return 0;
    }

    while (argc-- != 1) {
    
    /* open the file */
    printf("--- File %s ---\n",argv[filenum]);
    infile = fopen(argv[filenum],"r");
    if (!infile) {
        printf("cannot open file %s\n",argv[filenum]);
        return 1;
    }

    /* show file */
    while (1) {
        fgets(line,MAXLINE,infile);
        if (feof(infile))
            break;
        printf("%s",line);
        row++;
        if (row <24) continue;
        /* pause */
        printf("\n--- MORE ---");
        ch = fgetc(stdin);
        if (ch == 'q') {
            ch = fgetc(stdin);  // get terminating cr
            fclose(infile);
            printf("\n");
            return 0;
        }
        row = 0;
        continue;
    }
    fclose(infile);
    printf("\n");
    filenum += 1;
    continue;
    }

    return 0;
}


