/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere il programma nonest_symlink che ha come parametro il pathname di una directory.
Il programma deve cercare tutti i link simbolici presenti nella directory e cancelli ogni link simbolico
nidificato (link simbolico che punta a link simbolico).
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
 * Un symlink è "nidificato" se anche il file a cui punta direttamente
 * (un solo passo, senza risolvere l'intera catena) è a sua volta un
 * symlink: per questo basta lstat sul target letterale, non stat.
 */
int is_nested_symlink(const char *link_path) {
    char target[PATH_MAX];
    ssize_t n = readlink(link_path, target, sizeof target - 1);
    if (n == -1) {
        perror("readlink");
        return 0;
    }
    target[n] = '\0';

    char resolved[PATH_MAX];
    if (target[0] == '/') {
        strncpy(resolved, target, sizeof resolved - 1);
        resolved[sizeof resolved - 1] = '\0';
    } else {
        /* il target relativo va risolto rispetto alla directory che
         * contiene il link, non rispetto alla cwd del processo */
        char dirbuf[PATH_MAX];
        strncpy(dirbuf, link_path, sizeof dirbuf - 1);
        dirbuf[sizeof dirbuf - 1] = '\0';
        char *slash = strrchr(dirbuf, '/');
        if (slash != NULL) {
            *slash = '\0';
            if (snprintf(resolved, sizeof resolved, "%s/%s", dirbuf, target) >= (int) sizeof resolved)
                return 0;
        } else {
            strncpy(resolved, target, sizeof resolved - 1);
            resolved[sizeof resolved - 1] = '\0';
        }
    }

    struct stat st;
    if (lstat(resolved, &st) == -1) {
        /* target non esiste (link rotto): non è un caso di nidificazione */
        return 0;
    }

    return S_ISLNK(st.st_mode);
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

        if (S_ISLNK(st.st_mode) && is_nested_symlink(full_path)) {
            if (unlink(full_path) == -1)
                perror("unlink");
            else
                printf("Cancellato symlink nidificato: %s\n", full_path);
        }
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
