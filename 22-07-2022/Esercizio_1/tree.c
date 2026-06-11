/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Il programma da realizzare si chiama tree mostra un sottoalbero del file system.
 * es data una directory A che contiene una sottodirectory B e un file C; B contiene i file E F e G:
 * tree A
 * dovrebbe produrre
 * B
 *  E
 *  F
 *  G
 * C
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

static void tree(const char *path, int depth) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char child_path[PATH_MAX];
        snprintf(child_path, sizeof(child_path), "%s/%s", path, entry->d_name);

        for (int i = 0; i < depth; i++)
            printf(" ");
        printf("%s\n", entry->d_name);

        struct stat st;
        if (lstat(child_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode))
            tree(child_path, depth + 1);
    }

    if (closedir(dir) == -1)
        perror("closedir");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s non e' una directory\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    tree(argv[1], 0);

    return 0;
}
