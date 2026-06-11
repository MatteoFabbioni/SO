/**
Scrivere un programma che presi come parametri i pathname di
un file f e di una directory d stampi l'elenco dei link simbolici che puntano a f presenti nel sottoalbero
del file system generato dalla directory d
 */

#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

/* inode e device di f calcolati una sola volta nel main e passati come
 * parametri per evitare di rieseguire stat ad ogni ricorsione */
void walk(const char *d, ino_t t_ino, dev_t t_dev) {
    DIR *dir = opendir(d);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;

        char child[PATH_MAX];
        snprintf(child, sizeof child, "%s/%s", d, de->d_name);

        struct stat stl;
        /* lstat non segue symlink: ci dà info sull'entry stessa */
        if (lstat(child, &stl) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISLNK(stl.st_mode)) {
            /* stat segue il symlink e restituisce info sul file puntato:
             * se inode e device coincidono con quelli di f, il link punta a f.
             * stat fallisce su symlink pendenti (target inesistente): li ignoriamo */
            struct stat st_target;
            if (stat(child, &st_target) == 0)
                if (st_target.st_ino == t_ino && st_target.st_dev == t_dev)
                    printf("%s\n", child);
        }

        /* lstat non segue symlink a directory: evita loop infiniti su cicli */
        if (S_ISDIR(stl.st_mode))
            walk(child, t_ino, t_dev);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <directory>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) == -1) {
        perror("stat");
        return 1;
    }

    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Errore: %s non e' un file regolare\n", argv[1]);
        return 1;
    }

    walk(argv[2], st.st_ino, st.st_dev);
    return 0;
}
