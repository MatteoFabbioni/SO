/**
Esercizio 2: Linguaggio C: 10 punti
Scrivere il comando absls che mostri per ogni file della directory passata come parametro il path
completo di ogni file (mostrando al posto dei link simbolici il path completo dei file puntati).
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *dir_path = argv[1];

    struct stat dir_stat;
    if(stat(dir_path, &dir_stat) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if(!S_ISDIR(dir_stat.st_mode)){
        fprintf(stderr, "Error: %s is not a directory\n", dir_path);
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(dir_path);
    if(dir == NULL){
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_lstat;
        /* lstat per distinguere i symlink dalle altre entry senza seguirli */
        if(lstat(full_path, &entry_lstat) == -1){
            perror("lstat");
            continue;
        }

        if(S_ISLNK(entry_lstat.st_mode)){
            /* realpath risolve il symlink (anche a catena) nel path assoluto
             * del file puntato; un symlink pendente fa fallire realpath e
             * va semplicemente segnalato senza interrompere il listato */
            char resolved[PATH_MAX];
            if(realpath(full_path, resolved) != NULL){
                printf("%s\n", resolved);
            } else {
                fprintf(stderr, "%s: broken symlink\n", full_path);
            }
        } else {
            char resolved[PATH_MAX];
            if(realpath(full_path, resolved) != NULL){
                printf("%s\n", resolved);
            } else {
                perror("realpath");
            }
        }
    }

    closedir(dir);
    return 0;
}
