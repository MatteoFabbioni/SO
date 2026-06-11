/**
Esercizio 2: Linguaggio C: 10 punti
Il programma cprlt ha 3 paramentri: un tempo in secondi da epoch, una directory sorgente e una di
destinazione. es:
cprl 1689577839 a b
Il programma si deve comportare come cprl dell'esercizio 1 con la differenza che i file con tempo di
ultima modifica precedente al tempo indicato nel parametro vengono collegati con link fisici gli altri
devono essere copiati.
 */

#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

/* Copia il contenuto di src in dst byte per byte (i file con mtime non
 * precedente alla soglia vanno effettivamente copiati, non linkati) */
int copy_file(const char *src, const char *dst, mode_t mode) {
    int fd_in = open(src, O_RDONLY);
    if (fd_in == -1) {
        perror("open");
        return -1;
    }

    int fd_out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode & 0777);
    if (fd_out == -1) {
        perror("open");
        close(fd_in);
        return -1;
    }

    char buf[65536];
    ssize_t n;
    while ((n = read(fd_in, buf, sizeof buf)) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(fd_out, buf + off, n - off);
            if (w == -1) {
                perror("write");
                close(fd_in);
                close(fd_out);
                return -1;
            }
            off += w;
        }
    }
    if (n == -1)
        perror("read");

    close(fd_in);
    close(fd_out);
    return 0;
}

void walk(const char *src, const char *dst, time_t threshold) {
    struct stat st;
    if (lstat(src, &st) == -1) {
        perror("lstat");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        if (mkdir(dst, st.st_mode & 0777) == -1 && errno != EEXIST) {
            perror("mkdir");
            return;
        }

        DIR *dir = opendir(src);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        struct dirent *de;
        while ((de = readdir(dir)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            char child_src[PATH_MAX];
            char child_dst[PATH_MAX];
            snprintf(child_src, sizeof child_src, "%s/%s", src, de->d_name);
            snprintf(child_dst, sizeof child_dst, "%s/%s", dst, de->d_name);

            walk(child_src, child_dst, threshold);
        }

        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        if (st.st_mtime < threshold) {
            if (link(src, dst) == -1)
                perror("link");
        } else {
            copy_file(src, dst, st.st_mode);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <tempo_epoch> <a> <b>\n", argv[0]);
        return 1;
    }

    char *endptr;
    long t = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0') {
        fprintf(stderr, "Errore: '%s' non e' un tempo epoch valido\n", argv[1]);
        return 1;
    }
    time_t threshold = (time_t) t;

    struct stat st;
    if (lstat(argv[2], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Errore: %s non e' una directory\n", argv[2]);
        return 1;
    }

    walk(argv[2], argv[3], threshold);
    return 0;
}
