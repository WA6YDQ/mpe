/* toksplit.c test strtok()
 *
 * k theis 7/2020
 *
 */

#include <stdio.h>
#include <string.h>

int main (int argc, char **argv) {

	char ch[100];
	char* p;
	char* tok;
	char **args;

	while (1) {
		memset(ch,0,99);
		printf("Ok> ");
		fgets(ch,90,stdin);

		tok = ch;
		p = strtok(tok," ");
		if (strncmp(p,"exit",4)==0) return 0;

		puts(p);

		while(1) {
			tok = NULL;
			p = strtok(tok," ");
			if (!p) break;
			puts(p);
		}

	}



	return 0;
}


