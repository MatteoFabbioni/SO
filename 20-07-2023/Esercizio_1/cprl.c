/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un programma cprl che si comporti come il comando "cp -rl". cprl ha due parametri:
cprl a b
deve copiare l'intera struttura delle directory dell'albero che ha come radice a in un secondo albero
con radice b. I file non devono essere copiati ma collegati con link fisici.
(l'operazione deve essere fatta dal codice C, senza lanciare altri programmi/comandi)
 */

#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<dirent.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

void walk(const char *src, const char *dst) {
    struct stat st;
    if (lstat(src, &st) == -1) {
        perror("lstat");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        /* la modalita' della directory sorgente viene replicata su quella
         * di destinazione, come fa cp -r; se dst esiste gia' (es. radice b
         * creata da una chiamata precedente) non e' un errore fatale */
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

            walk(child_src, child_dst);
        }

        closedir(dir);
    } else if (S_ISREG(st.st_mode)) {
        if (link(src, dst) == -1)
            perror("link");
    }
    /* altri tipi di file (symlink, device, ecc.) non sono richiesti
     * dall'enunciato e vengono ignorati */
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <a> <b>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Errore: %s non e' una directory\n", argv[1]);
        return 1;
    }

    walk(argv[1], argv[2]);
    return 0;
}
