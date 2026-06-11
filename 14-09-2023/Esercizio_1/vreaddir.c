/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere la funzione:
char **vreaddir(const char *path)
che restituisca l'elenco dei nomi dei file in una directory come vettore di stringhe terminato con un
puntatore NULL. (lo stesso formato di argv o envp).
Il vettore e le stringhe dei nomi sono allocate dinamicamente.

completare l'esercizio con un programma principale che testi il corretto funzionamento della funzione
vreaddir.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

char **vreaddir(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }

    size_t count = 0;
    size_t capacity = 16;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dir);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (count + 1 >= capacity) {
            /* +1 per lo slot del terminatore NULL che va sempre garantito */
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                for (size_t i = 0; i < count; i++) free(names[i]);
                free(names);
                closedir(dir);
                return NULL;
            }
            names = tmp;
        }

        names[count] = malloc(strlen(entry->d_name) + 1);
        if (!names[count]) {
            perror("malloc");
            for (size_t i = 0; i < count; i++) free(names[i]);
            free(names);
            closedir(dir);
            return NULL;
        }
        strcpy(names[count], entry->d_name);
        count++;
    }

    names[count] = NULL;
    closedir(dir);
    return names;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a directory.\n", argv[1]);
        return 1;
    }

    char **names = vreaddir(argv[1]);
    if (!names) {
        fprintf(stderr, "vreaddir failed on %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0; names[i] != NULL; i++) {
        printf("%s\n", names[i]);
    }

    for (size_t i = 0; names[i] != NULL; i++) free(names[i]);
    free(names);

    return 0;
}
