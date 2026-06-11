/**
Esercizio 2: Linguaggio C: 10 punti

Estendere il programma precedente in modo che accetti le seguenti opzioni:
-c copia il file originale al posto di tutti i link
-l trasforma tutti i link in link fisici
-s trasforma tutti i link in link simbolici
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

/* copia il contenuto di f nel path dest, sostituendo l'entry che vi si trova:
 * va rimossa prima, perche' dest e' attualmente un link (fisico o simbolico) a f */
void copy_file(const char *src, const char *dest){
    int in_fd = open(src, O_RDONLY);
    if(in_fd == -1){
        perror("open");
        return;
    }

    if(unlink(dest) == -1){
        perror("unlink");
        close(in_fd);
        return;
    }

    int out_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(out_fd == -1){
        perror("open");
        close(in_fd);
        return;
    }

    char buf[4096];
    ssize_t n;
    while((n = read(in_fd, buf, sizeof(buf))) > 0){
        if(write(out_fd, buf, n) != n){
            perror("write");
            break;
        }
    }
    if(n == -1) perror("read");

    close(in_fd);
    close(out_fd);
}

/* sostituisce l'entry dest (link fisico o symlink a src) con un nuovo link
 * dello stesso tipo richiesto da new_type ('l' = fisico, 's' = simbolico) */
void relink(const char *src, const char *dest, char new_type){
    if(unlink(dest) == -1){
        perror("unlink");
        return;
    }

    if(new_type == 'l'){
        if(link(src, dest) == -1) perror("link");
    } else {
        if(symlink(src, dest) == -1) perror("symlink");
    }
}

void searchlink(const char *dir_path, const char *file_path, const char *file_real, ino_t target_ino, dev_t target_dev, char option){
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
        if(lstat(full_path, &entry_lstat) == -1){
            perror("lstat");
            continue;
        }

        /* se d contiene f stesso (es. d e' la directory che contiene f), questa
         * entry e' f e non un link ad f: va esclusa, altrimenti -c/-l/-s
         * finirebbero per cancellare o auto-collegare il file originale.
         * Un hard link e' una entry distinta con path canonico proprio (realpath
         * non lo "segue" verso nulla), quindi qui si esclude solo l'entry che e'
         * letteralmente f, non gli hard link che vi si riferiscono */
        if(!S_ISLNK(entry_lstat.st_mode)){
            char real_path[PATH_MAX];
            if(realpath(full_path, real_path) != NULL && strcmp(real_path, file_real) == 0){
                continue;
            }
        }

        int is_match = 0;
        if(S_ISLNK(entry_lstat.st_mode)){
            struct stat target_stat;
            if(stat(full_path, &target_stat) == 0)
                if(target_stat.st_ino == target_ino && target_stat.st_dev == target_dev)
                    is_match = 1;
        } else if(entry_lstat.st_ino == target_ino && entry_lstat.st_dev == target_dev){
            is_match = 1;
        }

        if(is_match){
            if(option == 'c')
                copy_file(file_path, full_path);
            else
                /* file_real e' il path assoluto canonico di f: necessario per
                 * creare un link valido indipendentemente dalla profondita' a
                 * cui si trova full_path nell'albero (un path relativo come
                 * quello passato su argv punterebbe altrove) */
                relink(file_real, full_path, option);
        }

        /* la ricorsione segue sempre l'albero originale tramite lstat, anche
         * dopo un'eventuale modifica dell'entry corrente: una entry e' o un
         * link verso f (quindi non una directory) oppure una directory vera,
         * mai entrambe le cose nello stesso passo */
        if(S_ISDIR(entry_lstat.st_mode)){
            searchlink(full_path, file_path, file_real, target_ino, target_dev, option);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr, "Usage: %s -c|-l|-s <file> <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *opt = argv[1];
    char *file_path = argv[2];
    char *dir_path = argv[3];

    if(strcmp(opt, "-c") != 0 && strcmp(opt, "-l") != 0 && strcmp(opt, "-s") != 0){
        fprintf(stderr, "Invalid option: %s\n", opt);
        exit(EXIT_FAILURE);
    }
    char option = opt[1];

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

    char file_real[PATH_MAX];
    if(realpath(file_path, file_real) == NULL){
        perror("realpath");
        exit(EXIT_FAILURE);
    }

    searchlink(dir_path, file_path, file_real, file_stat.st_ino, file_stat.st_dev, option);

    return 0;
}
