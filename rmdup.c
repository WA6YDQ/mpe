/*  rmdup.c  - remove duplicate elements from a file
 *
 *  k theis 7/2020
 *
 *  usage: rmdup [infile] [outfile]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mpe.h"



int main (int argc, char **argv) {

    int ct = 0;     // line counter
    char line[MAXLINE];
    FILE *infile, *outfile;

    if (argc != 3) {
        puts("usage: rmdup [infile] [outfile]");
        exit(0);
    }

    /* don't corrupt existing, fail if no files */
    if (access(argv[2],F_OK) == 0) {
        printf("file %s exists. Stopping\n",argv[2]);
        exit(1);
    }

    if (access(argv[1],F_OK) != 0) {
        printf("file %s not found. Stopping.\n",argv[1]);
        exit(1);
    }

    infile = fopen(argv[1],"r");
    if (!infile) {
        printf("cannot open %s\n",argv[1]);
        exit(1);
    }

    outfile = fopen(argv[2],"w");
    if (!outfile) {
        printf("cannot create %s\n",argv[2]);
        exit(1);
    }

    /* count elements */
    while (1) {
        fgets(line,MAXLINE,infile);
        if (feof(infile)) break;
        ct += 1;
    }
    rewind(infile);
    /* dynamically create storage */
    char elements[ct+1][MAXLINE];
    char copy_elements[ct+1][MAXLINE];

    int n=0;
    while (1) {
        fgets(line,MAXLINE,infile);
        if (feof(infile)) break;
        strcpy(elements[n++],line);
    }

    int uniqele = 0;
    /* only copy unique elements */
    for (n=0; n<ct; n++) {
        if (elements[n][0]=='\n' || elements[n][0]=='\0') continue;
        (strcpy(copy_elements[uniqele++],elements[n]));
        if (strcmp(elements[n],elements[n+1])==0)
            n++;
    }

    

    /* save sorted list */
    for (n=0; n<uniqele; n++)
        fprintf(outfile,"%s",copy_elements[n]);

    fflush(outfile);
    fclose(outfile);
    fclose(infile);

    printf("total %d elements\n",uniqele);

    exit(0);
}


