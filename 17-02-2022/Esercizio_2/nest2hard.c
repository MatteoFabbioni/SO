/**
Esercizio 2: Linguaggio C: 10 punti
Scrivere il programma nest2hard: che ha come parametro il pathname di una directory.
Il programma deve cercare tutti i link simbolici presenti nella directory e sostituire ogni link simbolico
nidificato (link simbolico che punta a link simbolico) con un link fisico del file puntato.
 */

#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<limits.h>
#include<sys/types.h>
#include<sys/stat.h>

/*
 * Risolve il target letterale (un solo passo) di un symlink, gestendo
 * path relativi rispetto alla directory che contiene il link stesso.
 */
int resolve_one_step(const char *link_path, char *out, size_t out_size) {
    char target[PATH_MAX];
    ssize_t n = readlink(link_path, target, sizeof target - 1);
    if (n == -1) {
        perror("readlink");
        return -1;
    }
    target[n] = '\0';

    if (target[0] == '/') {
        strncpy(out, target, out_size - 1);
        out[out_size - 1] = '\0';
        return 0;
    }

    char dirbuf[PATH_MAX];
    strncpy(dirbuf, link_path, sizeof dirbuf - 1);
    dirbuf[sizeof dirbuf - 1] = '\0';
    char *slash = strrchr(dirbuf, '/');
    if (slash != NULL) {
        *slash = '\0';
        snprintf(out, out_size, "%s/%s", dirbuf, target);
    } else {
        strncpy(out, target, out_size - 1);
        out[out_size - 1] = '\0';
    }
    return 0;
}

/*
 * Un symlink è "nidificato" se il suo target diretto è ancora un symlink.
 * In tal caso "il file puntato" da sostituire con un link fisico è il file
 * reale ottenuto seguendo l'intera catena (realpath risolve tutti i passi),
 * non il symlink intermedio.
 */
int is_nested_symlink(const char *link_path, char *real_target, size_t out_size) {
    char one_step[PATH_MAX];
    if (resolve_one_step(link_path, one_step, sizeof one_step) == -1)
        return 0;

    struct stat st;
    if (lstat(one_step, &st) == -1)
        return 0; /* target non esiste: link rotto, non e' nidificato */

    if (!S_ISLNK(st.st_mode))
        return 0;

    if (realpath(link_path, real_target) == NULL)
        return 0; /* catena con qualche anello rotto: non sostituibile */

    if ((int) out_size <= 0)
        return 0;

    return 1;
}

void scan_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof full_path, "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (!S_ISLNK(st.st_mode))
            continue;

        char real_target[PATH_MAX];
        if (!is_nested_symlink(full_path, real_target, sizeof real_target))
            continue;

        /* unlink prima del link: link(2) fallisce se full_path esiste già */
        if (unlink(full_path) == -1) {
            perror("unlink");
            continue;
        }
        if (link(real_target, full_path) == -1) {
            perror("link");
            continue;
        }
        printf("Sostituito symlink nidificato con link fisico: %s -> %s\n", full_path, real_target);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <pathname directory>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s non e' una directory\n", argv[1]);
        return 1;
    }

    scan_dir(argv[1]);

    return 0;
}
