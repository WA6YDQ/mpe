/*  date.c  k theis 7/2020
 *
 *  date - show date/time
 *
 *
*/

#include <stdio.h>
#include <time.h>
#include "mpe.h"

int main(void) {
char outstr[200];
time_t t;
struct tm *tmp;

        t = time(NULL);
        tmp = localtime(&t);
 
        if (tmp == NULL) {
            perror("localtime");
                return 1;
        }
        
        if (strftime(outstr, sizeof(outstr), "%A %D %I:%M %p", tmp) == 0) {
           fprintf(stderr, "strftime returned 0");
               return 1;
        }
        green();
        printf("\n%s UTC\n", outstr);
        white();
        return 0;
}
