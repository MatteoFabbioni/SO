/**
Scrivere un programma ckfile che dati il pathname di un file f e di una directory d stampi, a seconda
di una opzione:
•l'elenco dei link simbolici che puntano a f presenti nel sottoalbero del file system generato
dalla directory d, (opzione -s),
ckfile -s /tmp/file /tmp
•l'elenco dei link fisici di f presenti nel sottoalbero del file system generato dalla directory d
(opzione -l),
ckfile -l /tmp/file /tmp
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<dirent.h>
#include<string.h>
#include<limits.h>

/*
 * Visita ricorsivamente il sottoalbero radicato in dir_path.
 * Per ogni entry stampa il percorso se soddisfa la condizione richiesta:
 *   option == 's': link simbolici il cui target ha inode/dev == target_ino/target_dev
 *   option == 'l': file regolari (non symlink) con inode/dev == target_ino/target_dev
 * Usa lstat per non seguire i symlink durante la visita, stat solo dove serve.
 */
void list_symbolic_links(const char *dir_path, char option, ino_t target_ino, dev_t target_dev){
    DIR *dir = opendir(dir_path);
    if(dir == NULL){
        perror("opendir");
        return;
    }
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        /* salta le entry speciali . e .. per evitare loop infiniti */
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_lstat;
        /* lstat non segue i symlink: ci dà info sull'entry stessa */
        if(lstat(full_path, &entry_lstat) == -1){
            perror("lstat");
            continue;
        }

        if(option == 's' && S_ISLNK(entry_lstat.st_mode)){
            /* stat segue il symlink: ci dà info sul file puntato */
            struct stat entry_stat;
            if(stat(full_path, &entry_stat) == -1) continue; /* symlink pendente, skip */
            /* confronta inode e device per verificare che punti esattamente a f */
            if(entry_stat.st_ino == target_ino && entry_stat.st_dev == target_dev)
                printf("%s\n", full_path);
        } else if(option == 'l' && !S_ISLNK(entry_lstat.st_mode)){
            /* per i link fisici confrontiamo direttamente l'inode con lstat */
            if(entry_lstat.st_ino == target_ino && entry_lstat.st_dev == target_dev)
                printf("%s\n", full_path);
        }

        /* ricorre nelle sottodirectory usando lstat per non seguire symlink a dir */
        if(S_ISDIR(entry_lstat.st_mode)){
            list_symbolic_links(full_path, option, target_ino, target_dev);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "Usage: %s -s|-l <file> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *option   = argv[1];
    char *file_path = argv[2];
    char *dir_path  = argv[3];

    /* ottieni inode e device del file target per confrontarli in seguito */
    struct stat file_stat;
    if(stat(file_path, &file_stat) == -1){
        perror("stat");
        exit(EXIT_FAILURE);
    }

    /* il programma accetta solo file regolari come target */
    if(S_ISREG(file_stat.st_mode) == 0){
        fprintf(stderr, "Error: %s is not a regular file\n", file_path);
        exit(EXIT_FAILURE);
    }

    if(strcmp(option, "-s") == 0){
        printf("Symbolic links pointing to %s:\n", file_path);
        list_symbolic_links(dir_path, 's', file_stat.st_ino, file_stat.st_dev);
    } else if(strcmp(option, "-l") == 0){
        printf("Hard links to %s:\n", file_path);
        list_symbolic_links(dir_path, 'l', file_stat.st_ino, file_stat.st_dev);
    } else {
        fprintf(stderr, "Invalid option: %s\n", option);
        exit(EXIT_FAILURE);
    }

    return 0;
}
