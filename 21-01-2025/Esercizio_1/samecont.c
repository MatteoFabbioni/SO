/**
Scrivere il programma samecont che presi come
parametri i pathname di un file f e di una directory d stampi l'elenco dei file che hanno la stessa
ampiezza (numero di byte) di f ma non sono link fisici di f presenti nel sottoalbero del file system
generato dalla directory d
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

/* inode, device e dimensione di f calcolati una sola volta nel main e passati
 * come parametri per evitare di rieseguire realpath+stat ad ogni ricorsione */
void walk(const char *d, ino_t t_ino, dev_t t_dev, off_t t_size) {
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

        struct stat st1;
        /* lstat non segue symlink: evita crash su symlink pendenti e loop su
         * symlink a directory */
        if (lstat(child, &st1) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISREG(st1.st_mode)) {
            /* un hard link di f condivide inode E device; controllare solo l'inode
             * non basta se la visita attraversa più filesystem */
            int is_hardlink = (st1.st_ino == t_ino && st1.st_dev == t_dev);
            if (!is_hardlink && st1.st_size == t_size)
                printf("%s\n", child);
        }

        if (S_ISDIR(st1.st_mode))
            walk(child, t_ino, t_dev, t_size);
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

    walk(argv[2], st.st_ino, st.st_dev, st.st_size);
    return 0;
}
