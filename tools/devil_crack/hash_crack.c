# tools/devil_crack/hash_crack.c
// Password hash cracker

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

typedef unsigned char uint8_t;

static void md5_crack(const char *hash, const char *wordlist) {
    FILE *f = fopen(wordlist, "r");
    if (!f) {
        printf("Cannot open %s\n", wordlist);
        return;
    }
    
    char word[256];
    while (fgets(word, sizeof(word), f)) {
        word[strcspn(word, "\n")] = 0;
        
        uint8_t md[16];
        MD5((uint8_t*)word, strlen(word), md);
        
        char out[33];
        for (int i = 0; i < 16; i++) {
            sprintf(out + i*2, "%02x", md[i]);
        }
        
        if (!strcmp(out, hash)) {
            printf("Found: %s\n", word);
            fclose(f);
            return;
        }
    }
    
    printf("Password not found in wordlist\n");
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: devil_crack <hash> <wordlist>\n");
        return 1;
    }
    
    md5_crack(argv[1], argv[2]);
    return 0;
}