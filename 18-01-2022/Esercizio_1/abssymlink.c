/**
Scrivere il programma abssymlink che ha come parametro il pathname di una directory.
Il programma deve cercare tutti i link simbolici presenti nella directory e trasformare ogni link
simbolico in uno equivalente che punti al pathname assoluto e non relativo..
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * Se il target del link è relativo, va risolto rispetto alla directory che
 * contiene il link (non rispetto alla directory corrente del processo),
 * altrimenti realpath() lo interpreterebbe in modo sbagliato.
 */
static int make_link_absolute(const char *dir_path, const char *entry_name) {
    char link_path[PATH_MAX];
    if (snprintf(link_path, sizeof link_path, "%s/%s", dir_path, entry_name) >= (int) sizeof link_path) {
        fprintf(stderr, "Errore: pathname troppo lungo\n");
        return -1;
    }

    char target[PATH_MAX];
    ssize_t n = readlink(link_path, target, sizeof target - 1);
    if (n == -1) {
        perror("readlink");
        return -1;
    }
    target[n] = '\0';

    if (target[0] == '/') {
        /* è già assoluto, niente da fare */
        return 0;
    }

    char resolved_dir[PATH_MAX];
    if (realpath(dir_path, resolved_dir) == NULL) {
        perror("realpath");
        return -1;
    }

    char candidate[PATH_MAX];
    if (snprintf(candidate, sizeof candidate, "%s/%s", resolved_dir, target) >= (int) sizeof candidate) {
        fprintf(stderr, "Errore: pathname troppo lungo\n");
        return -1;
    }

    char abs_target[PATH_MAX];
    if (realpath(candidate, abs_target) == NULL) {
        perror("realpath");
        return -1;
    }

    if (unlink(link_path) == -1) {
        perror("unlink");
        return -1;
    }

    if (symlink(abs_target, link_path) == -1) {
        perror("symlink");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory>\n", argv[0]);
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

    DIR *d = opendir(argv[1]);
    if (d == NULL) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    int had_error = 0;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char entry_path[PATH_MAX];
        if (snprintf(entry_path, sizeof entry_path, "%s/%s", argv[1], entry->d_name) >= (int) sizeof entry_path) {
            fprintf(stderr, "Errore: pathname troppo lungo\n");
            had_error = 1;
            continue;
        }

        struct stat entry_st;
        if (lstat(entry_path, &entry_st) == -1) {
            perror("lstat");
            had_error = 1;
            continue;
        }

        if (S_ISLNK(entry_st.st_mode)) {
            if (make_link_absolute(argv[1], entry->d_name) == -1)
                had_error = 1;
        }
    }

    closedir(d);
    return had_error ? 1 : 0;
}
