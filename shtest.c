#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {

    if (argc == 1) {
        printf("%s:no args\n",argv[0]);
        return 0;
    }
    printf("%s: %d args\n",argv[0],argc);
    
    while (argc--)
        printf("[%s] ",*argv++);

    printf("\n");
    return 0;
}
