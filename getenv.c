/* getenv.c  - get environment variables
 *
 * k theis 7/2020
 *
 */


#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    char **env;
    for(env = envp; *env != 0; env++)
    {
        char *evar = *env;
        printf("%s\n", evar);
    }
    return 0;
}

