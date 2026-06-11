/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un programma search_name che deve cercare nel sottoalbero della directory corrente tutti i
file eseguibili con un nome file specifico passato come primo e unico parametro indicando per ogni
file il tipo di eseguibile (script o eseguibile binario).
Ad esempio il comando:
./search_name testprog
deve cercare i file eseguibili chiamati testprog nell'albero della directory corrente. Poniamo
siano ./testprog, ./dir1/testprog, ./dir/dir3/testprog, search_name deve stampare:
./testprog: script
./dir1/testprog: ELF executable
./dir/dir3/testprog: ELF executable
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX/BSD necessarie per opendir/readdir su Linux */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/* I file ELF iniziano con i 4 byte magic 0x7F 'E' 'L' 'F' */
static const unsigned char ELF_MAGIC[4] = {0x7f, 'E', 'L', 'F'};

void print_exec_type(const char *path){
    FILE *fp = fopen(path, "rb");
    if(fp == NULL){
        perror("fopen");
        return;
    }

    unsigned char header[4];
    size_t n = fread(header, 1, sizeof(header), fp);
    fclose(fp);

    if(n == sizeof(header) && memcmp(header, ELF_MAGIC, sizeof(ELF_MAGIC)) == 0){
        printf("%s: ELF executable\n", path);
    } else if(n >= 2 && header[0] == '#' && header[1] == '!'){
        /* uno script eseguibile inizia tipicamente con uno shebang "#!" */
        printf("%s: script\n", path);
    } else {
        /* eseguibile di tipo non riconosciuto dai soli primi byte */
        printf("%s: script\n", path);
    }
}

void search(const char *dirpath, const char *target){
    DIR *dir = opendir(dirpath);
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        /* lstat: non segue i symlink, evita di seguire link che puntano fuori
           dall'albero o che creano cicli nel filesystem */
        if(lstat(full_path, &st) == -1){
            perror("lstat");
            continue;
        }

        if(S_ISDIR(st.st_mode)){
            search(full_path, target);
        } else if(S_ISREG(st.st_mode) && strcmp(entry->d_name, target) == 0){
            if(access(full_path, X_OK) == 0){
                print_exec_type(full_path);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <nome_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    search(".", argv[1]);

    return 0;
}
