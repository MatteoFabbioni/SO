/**
Scrivere il programma cpsymlink: che ha come parametro il pathname di una directory.
Il programma deve cercare tutti i link simbolici presenti nella directory e sostituire ogni link simbolico
che punta ad un file regolare con la copia del file puntato.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 * stat() (non lstat) qui è voluto: dobbiamo seguire il link per sapere se il
 * file puntato è un file regolare e per leggerne il contenuto.
 */
static int replace_link_with_copy(const char *link_path) {
    struct stat target_st;
    if (stat(link_path, &target_st) == -1) {
        /* link rotto: target non esiste o non è raggiungibile, si salta */
        fprintf(stderr, "%s: target non valido, salto\n", link_path);
        return -1;
    }

    if (!S_ISREG(target_st.st_mode)) {
        /* il link non punta a un file regolare: niente da fare */
        return 0;
    }

    int in_fd = open(link_path, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return -1;
    }

    char tmp_path[PATH_MAX];
    if (snprintf(tmp_path, sizeof tmp_path, "%s.cpsymlink.tmp", link_path) >= (int) sizeof tmp_path) {
        fprintf(stderr, "Errore: pathname troppo lungo\n");
        close(in_fd);
        return -1;
    }

    int out_fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC, target_st.st_mode & 0777);
    if (out_fd == -1) {
        perror("open");
        close(in_fd);
        return -1;
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(in_fd, buf, sizeof buf)) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(out_fd, buf + written, n - written);
            if (w == -1) {
                perror("write");
                close(in_fd);
                close(out_fd);
                unlink(tmp_path);
                return -1;
            }
            written += w;
        }
    }
    if (n == -1) {
        perror("read");
        close(in_fd);
        close(out_fd);
        unlink(tmp_path);
        return -1;
    }

    close(in_fd);
    close(out_fd);

    /* rename sul file temporaneo rimpiazza atomicamente il symlink con la copia */
    if (rename(tmp_path, link_path) == -1) {
        perror("rename");
        unlink(tmp_path);
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
            if (replace_link_with_copy(entry_path) == -1)
                had_error = 1;
        }
    }

    closedir(d);
    return had_error ? 1 : 0;
}
