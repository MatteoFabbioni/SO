/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere un programma che crei nella directory corrente (se non esiste già) una sottodirectory di
nome ... (tre punti).
Tutti i file (regolari) presenti nella directory devono essere spostati nella sottodirectory ... (tre punti) e
ogni file deve essere sostituito nella dir corrente con un link simbolico (relativo, non assoluto) alla
nuova locazione. Usare la system call rename per fare la sostituzione in modo atomico (in nessun
istante il file deve risultare inesistente).
 */

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<dirent.h>
#include<errno.h>
#include<limits.h>
#include<sys/types.h>
#include<sys/stat.h>

#define DEST_DIR "..."

/*
 * Sposta il file regolare name (presente nella directory corrente) dentro
 * DEST_DIR con rename (atomico: il file esiste sempre, prima nella vecchia
 * posizione, poi nella nuova). Solo dopo che lo spostamento è completo si
 * crea, con un secondo rename, il symlink al posto del nome originale: in
 * nessun momento "name" risulta inesistente.
 */
void move_and_link(const char *name) {
    char dest_path[PATH_MAX];
    if (snprintf(dest_path, sizeof dest_path, "%s/%s", DEST_DIR, name) >= (int) sizeof dest_path) {
        fprintf(stderr, "Errore: pathname troppo lungo per %s\n", name);
        return;
    }

    if (rename(name, dest_path) == -1) {
        perror("rename");
        return;
    }

    /* link relativo: dalla dir corrente il file si trova in .../name */
    char link_target[PATH_MAX];
    if (snprintf(link_target, sizeof link_target, "%s/%s", DEST_DIR, name) >= (int) sizeof link_target) {
        fprintf(stderr, "Errore: pathname troppo lungo per %s\n", name);
        return;
    }

    /* il symlink va creato con un nome temporaneo e poi rinominato sopra
     * "name": in questo modo non esiste un istante in cui "name" non punti
     * a nessun contenuto valido (prima il file vero, poi subito il link) */
    char tmp_link[PATH_MAX];
    if (snprintf(tmp_link, sizeof tmp_link, "%s.tmplink", name) >= (int) sizeof tmp_link) {
        fprintf(stderr, "Errore: pathname troppo lungo per %s\n", name);
        return;
    }

    if (symlink(link_target, tmp_link) == -1) {
        perror("symlink");
        return;
    }

    if (rename(tmp_link, name) == -1) {
        perror("rename");
        unlink(tmp_link);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "Uso: %s\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(DEST_DIR, &st) == -1) {
        if (errno != ENOENT) {
            perror("stat");
            return 1;
        }
        if (mkdir(DEST_DIR, 0777) == -1) {
            perror("mkdir");
            return 1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s esiste già e non è una directory\n", DEST_DIR);
        return 1;
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    /* l'elenco dei file regolari va raccolto prima di muoverli: spostare
     * voci mentre si scandisce la stessa directory con readdir può far
     * saltare o ripetere entry, comportamento non specificato da POSIX */
    int capacity = 64;
    int n_files = 0;
    char **files = malloc(capacity * sizeof(char *));
    if (files == NULL) {
        perror("malloc");
        closedir(dir);
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (strcmp(entry->d_name, DEST_DIR) == 0)
            continue;

        struct stat est;
        if (lstat(entry->d_name, &est) == -1) {
            perror("lstat");
            continue;
        }
        if (!S_ISREG(est.st_mode))
            continue;

        if (n_files == capacity) {
            capacity *= 2;
            char **grown = realloc(files, capacity * sizeof(char *));
            if (grown == NULL) {
                perror("realloc");
                break;
            }
            files = grown;
        }
        files[n_files] = strdup(entry->d_name);
        if (files[n_files] == NULL) {
            perror("strdup");
            break;
        }
        n_files++;
    }
    closedir(dir);

    for (int i = 0; i < n_files; i++) {
        move_and_link(files[i]);
        free(files[i]);
    }
    free(files);

    return 0;
}
