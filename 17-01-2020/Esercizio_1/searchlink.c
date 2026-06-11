/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti.
Scrivere un programma searchlink che dati due parametri (nell'ordine un file f ed una directory d)
metta in output l'elenco dei path all'interno dell'albero che ha radice in d che fanno riferimento ad f
o come link fisico o come link simbolico.

es:
searchlink myfile mydir
link a
link d/b
symlink e/s

significa che dir/a e dir/d/b si riferisono a f come link fisici mentre dir/e/s e' un link simbolico che punta
ad f.
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

void searchlink(const char *dir_path, ino_t target_ino, dev_t target_dev){
    DIR *dir = opendir(dir_path);
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_lstat;
        /* lstat non segue i symlink: serve per individuarli come tali */
        if(lstat(full_path, &entry_lstat) == -1){
            perror("lstat");
            continue;
        }

        if(S_ISLNK(entry_lstat.st_mode)){
            /* stat segue il symlink per scoprire a cosa punta davvero;
             * un symlink pendente fallisce stat e va semplicemente ignorato */
            struct stat target_stat;
            if(stat(full_path, &target_stat) == 0)
                if(target_stat.st_ino == target_ino && target_stat.st_dev == target_dev)
                    printf("symlink %s\n", full_path);
        } else if(entry_lstat.st_ino == target_ino && entry_lstat.st_dev == target_dev){
            printf("link %s\n", full_path);
        }

        /* la ricorsione si basa su lstat: una sottodirectory raggiunta solo
         * tramite symlink non viene riattraversata, evitando cicli */
        if(S_ISDIR(entry_lstat.st_mode)){
            searchlink(full_path, target_ino, target_dev);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Usage: %s <file> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *file_path = argv[1];
    char *dir_path = argv[2];

    struct stat file_stat;
    if(stat(file_path, &file_stat) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if(!S_ISREG(file_stat.st_mode)){
        fprintf(stderr, "Error: %s is not a regular file\n", file_path);
        exit(EXIT_FAILURE);
    }

    struct stat dir_stat;
    if(stat(dir_path, &dir_stat) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if(!S_ISDIR(dir_stat.st_mode)){
        fprintf(stderr, "Error: %s is not a directory\n", dir_path);
        exit(EXIT_FAILURE);
    }

    searchlink(dir_path, file_stat.st_ino, file_stat.st_dev);

    return 0;
}
