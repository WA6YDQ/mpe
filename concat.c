/* concat.c - add file2 to the end of file1
 *
 * k theis 7/2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mpe.h"

int main(int argc, char **argv) {
int files = 2;

    if (argc < 3) {
        puts("usage: concat [file1] [file2] ... ");
        exit(0);
    }
    
    if (access(argv[1],F_OK)) {
        printf("cannot open %s\n",argv[1]);
        exit(1);
    }

	/* loop thru arguements, adding files 2 thru n to file1 */
	while (argc-- > 2) {

    if (access(argv[files],F_OK)) {
        printf("cannot open %s\n",argv[files]);
		files++;
		continue;
    }

    char c[1024];   // buffer
    FILE *file1, *file2;
    file1 = fopen(argv[1],"a");
    if (!file1) {
        printf("error opening %s\n",argv[1]);
        exit(1);
    }

    file2 = fopen(argv[files],"r");
    if (!file2) {
        fclose(file1);
        printf("error opening %s\n",argv[files]);
		files++;
		continue;
    }

    while (!feof(file2)) {
        size_t bytes = fread(c, 1, sizeof(c), file2);
        if (bytes)
            fwrite(c, 1, bytes, file1);
    }

    fflush(file1);
    fclose(file2);
    fclose(file1);
	files++;
	continue;
	}

    return 0;
}


