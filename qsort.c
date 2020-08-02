/*  sort.c  - file sorter
 *
 *  k theis 7/2020
 *
 *  usage: sort [infile] [outfile]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mpe.h"

/********* qsort compare routine **********/
static int cmpstringp(const void *p1, const void *p2) {     // for qsort
    //printf("comparing %s  %s\n",(char *)p1,(char *)p2);
    return strcmp((char *) p1, (char *) p2);        // alpha compare
}



int main (int argc, char **argv) {

    int ct = 0;     // line counter
    char line[MAXLINE];
    FILE *infile, *outfile;

    if (argc != 3) {
        puts("usage: sort [infile] [outfile]");
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

    int n=0;
    while (1) {
        fgets(line,MAXLINE,infile);
        if (feof(infile)) break;
        strcpy(elements[n++],line);
    }

    /* sort elements */
    qsort(&elements[0],ct,MAXLINE,cmpstringp);

    /* save sorted list */
    for (n=0; n<ct; n++)
        fprintf(outfile,"%s",elements[n]);

    fflush(outfile);
    fclose(outfile);
    fclose(infile);

    printf("sorted %d elements\n",ct);

    exit(0);
}


