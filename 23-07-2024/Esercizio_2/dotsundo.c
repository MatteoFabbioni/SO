/**
Esercizio 2: Linguaggio C: 10 punti
(undo dell'esercizio 1) Scrivere un programma che sostituisca tutti i link simbolici presenti nella
directory corrente che puntano a .../nomefile con i veri file che l'esercizio 1 aveva spostato nella
directory tre punti. Usare la system call rename per fare la sostituzione in modo atomico (in nessun
istante il file deve risultare inesistente).
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<limits.h>
#include<sys/types.h>
#include<sys/stat.h>

#define SRC_DIR "..."
#define SRC_PREFIX SRC_DIR "/"

/*
 * Verifica che il symlink name punti a ".../nomefile" (prefisso letterale,
 * come scritto dall'esercizio 1) e, in tal caso, sposta il vero file dalla
 * sottodirectory "..." al posto del link con un singolo rename: l'operazione
 * è atomica, quindi "name" non risulta mai inesistente.
 */
void undo_link(const char *name) {
    char target[PATH_MAX];
    ssize_t n = readlink(name, target, sizeof target - 1);
    if (n == -1) {
        perror("readlink");
        return;
    }
    target[n] = '\0';

    if (strncmp(target, SRC_PREFIX, strlen(SRC_PREFIX)) != 0)
        return;

    if (rename(target, name) == -1)
        perror("rename");
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "Uso: %s\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(SRC_DIR, &st) == -1) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s non è una directory\n", SRC_DIR);
        return 1;
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    /* l'elenco dei link va raccolto prima di rinominarli: sostituire voci
     * mentre si scandisce la stessa directory con readdir può far saltare
     * o ripetere entry, comportamento non specificato da POSIX */
    int capacity = 64;
    int n_links = 0;
    char **links = malloc(capacity * sizeof(char *));
    if (links == NULL) {
        perror("malloc");
        closedir(dir);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (strcmp(entry->d_name, SRC_DIR) == 0)
            continue;

        struct stat est;
        if (lstat(entry->d_name, &est) == -1) {
            perror("lstat");
            continue;
        }
        if (!S_ISLNK(est.st_mode))
            continue;

        if (n_links == capacity) {
            capacity *= 2;
            char **grown = realloc(links, capacity * sizeof(char *));
            if (grown == NULL) {
                perror("realloc");
                break;
            }
            links = grown;
        }
        links[n_links] = strdup(entry->d_name);
        if (links[n_links] == NULL) {
            perror("strdup");
            break;
        }
        n_links++;
    }
    closedir(dir);

    for (int i = 0; i < n_links; i++) {
        undo_link(links[i]);
        free(links[i]);
    }
    free(links);

    return 0;
}
