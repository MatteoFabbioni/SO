/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scrivere il programma invsymlink che prende come parametro il pathname di un file.
Se il parametro è un link simbolico il programma inverte il link simbolico: il file puntato viene messo al
posto del file link simbolico e nella precedente collocazione dove era il file puntato dal link simbolico
viene messo un link simbolico che punti alla nuova collocazione del file. (*)
Se A e B indicano lo stesso file perché B è un link simbolico che punta ad A, allora "invsymlink B" deve
fare in modo che B diventi il file che precedentemente era A e A diventi un link simbolico che punti a
B.
(*) usare il pathname completo che può essere calcolato con realpath(3).

Esercizio 2: Linguaggio C: 10 punti
Estendere il programma invsymlink: se il parametro è una directory e non un file allora tutti i link
simbolici presenti nella directory devono venir "invertititi".
 */

#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<libgen.h>
#include<limits.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>

/*
 * Calcola il pathname assoluto di link_path senza seguire il link stesso:
 * realpath() seguirebbe il symlink fino al target, quindi risolviamo solo
 * la directory che lo contiene (che esiste sempre ed è un file vero) e
 * vi riattacchiamo il basename originale.
 */
int abs_path_of_link(const char *link_path, char *out, size_t out_size) {
    char tmp[PATH_MAX];
    strncpy(tmp, link_path, sizeof tmp - 1);
    tmp[sizeof tmp - 1] = '\0';
    char *base = basename(tmp);

    char tmp2[PATH_MAX];
    strncpy(tmp2, link_path, sizeof tmp2 - 1);
    tmp2[sizeof tmp2 - 1] = '\0';
    char *dir = dirname(tmp2);

    char dir_real[PATH_MAX];
    if (realpath(dir, dir_real) == NULL) {
        perror("realpath");
        return -1;
    }

    if (snprintf(out, out_size, "%s/%s", dir_real, base) >= (int) out_size) {
        fprintf(stderr, "Errore: pathname troppo lungo\n");
        return -1;
    }
    return 0;
}

/*
 * Inverte il symlink b_path: il file da esso puntato (a_path, calcolato a
 * partire dal contenuto del link, risolvendo i path relativi rispetto alla
 * directory di b_path) prende il posto di b_path, e al posto di a_path
 * viene creato un nuovo symlink che punta alla nuova collocazione (b_path).
 */
int invert_symlink(const char *b_path) {
    struct stat stb;
    if (lstat(b_path, &stb) == -1) {
        perror("lstat");
        return -1;
    }
    if (!S_ISLNK(stb.st_mode)) {
        fprintf(stderr, "%s non e' un link simbolico, salto\n", b_path);
        return -1;
    }

    char b_abs[PATH_MAX];
    if (abs_path_of_link(b_path, b_abs, sizeof b_abs) == -1)
        return -1;

    /* readlink legge il contenuto letterale del link (può essere relativo) */
    char link_target[PATH_MAX];
    ssize_t n = readlink(b_path, link_target, sizeof link_target - 1);
    if (n == -1) {
        perror("readlink");
        return -1;
    }
    link_target[n] = '\0';

    /* se il target è relativo va risolto rispetto alla directory di B,
     * non rispetto alla directory corrente del processo */
    char candidate[PATH_MAX];
    if (link_target[0] == '/') {
        strncpy(candidate, link_target, sizeof candidate - 1);
        candidate[sizeof candidate - 1] = '\0';
    } else {
        char tmp[PATH_MAX];
        strncpy(tmp, b_path, sizeof tmp - 1);
        tmp[sizeof tmp - 1] = '\0';
        char *bdir = dirname(tmp);
        if (snprintf(candidate, sizeof candidate, "%s/%s", bdir, link_target) >= (int) sizeof candidate) {
            fprintf(stderr, "Errore: pathname troppo lungo\n");
            return -1;
        }
    }

    char a_abs[PATH_MAX];
    if (realpath(candidate, a_abs) == NULL) {
        perror("realpath");
        return -1;
    }

    /* rename sovrascrive l'eventuale file/symlink già presente in b_abs,
     * quindi cancella il vecchio symlink B mettendo al suo posto il file A */
    if (rename(a_abs, b_abs) == -1) {
        perror("rename");
        return -1;
    }

    /* al posto precedentemente occupato da A creiamo il nuovo symlink verso B */
    if (symlink(b_abs, a_abs) == -1) {
        perror("symlink");
        return -1;
    }

    return 0;
}

/*
 * Scandisce dir_path (un solo livello, non ricorsivamente: l'enunciato
 * richiede solo i link simbolici "presenti nella directory") e inverte
 * ogni link simbolico trovato. Usiamo lstat sulle entry per non seguire
 * eventuali symlink a directory durante la scansione.
 *
 * L'elenco dei symlink viene raccolto in un primo passaggio completo su
 * readdir e solo dopo si esegue l'inversione: invertendo un link, il file
 * che prima era il target diventa a sua volta un symlink (con un nome
 * diverso ma nella stessa directory), quindi modificare durante la stessa
 * scansione rischierebbe di incontrarlo di nuovo e invertirlo due volte,
 * annullando l'effetto. L'elenco è allocato dinamicamente (non sullo
 * stack) perché il numero di link in una directory non è limitato a priori.
 */
void invert_dir(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    int capacity = 64;
    int n_links = 0;
    char **links = malloc(capacity * sizeof(char *));
    if (links == NULL) {
        perror("malloc");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof full_path, "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISLNK(st.st_mode)) {
            if (n_links == capacity) {
                capacity *= 2;
                char **grown = realloc(links, capacity * sizeof(char *));
                if (grown == NULL) {
                    perror("realloc");
                    break;
                }
                links = grown;
            }
            links[n_links] = strdup(full_path);
            if (links[n_links] == NULL) {
                perror("strdup");
                break;
            }
            n_links++;
        }
    }

    closedir(dir);

    for (int i = 0; i < n_links; i++) {
        invert_symlink(links[i]);
        free(links[i]);
    }
    free(links);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <pathname>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }

    if (S_ISDIR(st.st_mode)) {
        invert_dir(argv[1]);
    } else if (S_ISLNK(st.st_mode)) {
        if (invert_symlink(argv[1]) == -1)
            return 1;
    } else {
        fprintf(stderr, "%s non e' né un link simbolico né una directory\n", argv[1]);
        return 1;
    }

    return 0;
}
