/* slist.c  file lister (a la primos)
 *
 * k theis 7/2020
 *
 */

#include <stdio.h>
#include <string.h>

int main (int argc, char** argv) {
FILE *infile;
char line[256];

	if (argc == 1) {
		puts("usage: slist file");
		return 0;
	}

	infile = fopen(argv[1],"r");
	if (!infile) {
		printf("cannot open %s\n",argv[1]);
		return 1;
	}

	while (1) {
		memset(line,0,255);
		fgets(line,255,infile);
		if (feof(infile)) break;
		printf("%s",line);
	}

	fclose(infile);
	puts("");
	return 0;
}


