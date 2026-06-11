/**
 * Esercizio 2: Linguaggio C: (alternativo a 1) 30 punti
 * La particolarità di 'tree' per l'esecizio 2 è che si chiede che venga implementato con una sola
 * funzione ricorsiva, senza concatenare stringhe o cambiare directory. Tree va implementato tramite
 * openat e fdopendir.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* dirfd resta apribile come fd di partenza per openat/fstatat: cosi' non serve
 * mai concatenare path o fare chdir, ogni livello lavora relativo al fd del padre. */
static void tree(int dirfd, int depth) {
    int fd_copy = openat(dirfd, ".", O_RDONLY | O_DIRECTORY);
    if (fd_copy == -1) {
        perror("openat");
        return;
    }

    DIR *dir = fdopendir(fd_copy);
    if (dir == NULL) {
        perror("fdopendir");
        close(fd_copy);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        for (int i = 0; i < depth; i++)
            printf(" ");
        printf("%s\n", entry->d_name);

        struct stat st;
        if (fstatat(fd_copy, entry->d_name, &st, AT_SYMLINK_NOFOLLOW) == -1) {
            perror("fstatat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            int child_fd = openat(fd_copy, entry->d_name, O_RDONLY | O_DIRECTORY);
            if (child_fd == -1) {
                perror("openat");
                continue;
            }
            tree(child_fd, depth + 1);
            if (close(child_fd) == -1)
                perror("close");
        }
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

    int fd = open(argv[1], O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    tree(fd, 0);

    if (close(fd) == -1)
        perror("close");

    return 0;
}
