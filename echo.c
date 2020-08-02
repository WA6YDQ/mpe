/* echo.c - echo command line arguements 
 *
 * k theis 7/2020
 *
 */

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {

	if (argc == 1) {
		printf("\n");
		return 0;
	}

	argv++;
	while (--argc) 
		printf("%s ",*argv++);


	printf("\n");
	return 0;
}

