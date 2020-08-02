/*  crypt.c - use rc4 to encrypt/decrypt files
 *  
 *  mostly written by:
 *   robin verton, dec 2015
 *   implementation of the RC4 algo
 *	
 *
 *	wrote functions to read/write files,
 *	fix minor issues
 *	k theis 7/2020
 * 
 *  key can be any alphanumeric char from 4-256 digits
 *  in length, the longer the better.
 *
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#define N 256   // 2^8
int filesize = 0;		// global, used instead of original strlen in PRGA()

void swap(unsigned char *a, unsigned char *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void KSA(char *key, unsigned char *S) {

    int len = strlen(key);
    int j = 0;

    for(int i = 0; i < N; i++)
        S[i] = i;

    for(int i = 0; i < N; i++) {
        j = (j + S[i] + key[i % len]) % N;

        swap(&S[i], &S[j]);
    }

    return;
}

void PRGA(unsigned char *S, char *plaintext, unsigned char *ciphertext) {

    int i = 0;
    int j = 0;

    for(size_t n = 0, len = filesize; n < len; n++) {
        i = (i + 1) % N;
        j = (j + S[i]) % N;

        swap(&S[i], &S[j]);
        int rnd = S[(S[i] + S[j]) % N];

        ciphertext[n] = rnd ^ plaintext[n];

    }

    return;
}

void RC4(char *key, char *plaintext, unsigned char *ciphertext) {

    unsigned char S[N];
    KSA(key, S);

    PRGA(S, plaintext, ciphertext);

    return;
}

int main(int argc, char *argv[]) {
FILE *infile, *outfile;
struct stat buf;
int err = 0;

	if (argc != 4) {
		printf("usage: %s key infile outfile\n",basename(argv[0]));
		printf("Symetric rc4-based en/decryption program. Use quotes\n");
		printf("on key if key contains spaces. Key length < 256\n");
		exit(0);
	}

	infile = fopen(argv[2],"r");
	if (infile == NULL) {
		printf("cannot open %s\n",argv[2]);
		exit(1);
	}

	
	if (access(argv[3],F_OK) ==0) {
		printf("file %s exists. Stopping.\n",argv[3]);
		exit(1);
	}


	err = lstat(argv[2], &buf);
	if (err == -1) {
		perror("lstat");
		exit(1);
	}
	filesize = buf.st_size;

	outfile = fopen(argv[3],"w");
	if (outfile == NULL) {
		printf("%s\n",argv[3]);
		perror("");
		exit(1);
	}

	int pos = 0;
	unsigned char* ptext = malloc(filesize+1);
	if (!ptext) {
		puts("memory error");
		exit(1);
	}

	/* read infile to buffer */
	while (pos < filesize) {
		ptext[pos++] = fgetc(infile);
	}
	--pos;
	fclose(infile);
    
	unsigned char *ciphertext = malloc(sizeof(int) * pos);
	if (!ciphertext) {
		puts("memory error");
		exit(1);
	}

	/* key, infile, outfile */
    RC4(argv[1], ptext, ciphertext);

	/* write ciphertext to file */
    for(size_t i = 0; i < pos; i++) {
		fprintf(outfile,"%c",ciphertext[i]);
	}

	fflush(outfile);
	fclose(outfile);
    return 0;
}
