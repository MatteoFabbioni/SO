/**
 * Estendere il programma dell'esercizio 1 con ulteriori opzioni per stampare:
 *
 * • l'elenco dei file che hanno lo stesso contenuto di f presenti nel sottoalbero
 *   del file system generato dalla directory d (privo di opzione):
 *       ckfile /tmp/file /tmp
 *
 * • l'elenco dei file presenti nel sottoalbero del file system generato dalla
 *   directory d che hanno come contenuto la parte iniziale del file f
 *   (opzione -p seguita dalla lunghezza del prefisso comune):
 *       ckfile -p 100 /tmp/file /tmp
 *   stampa l'elenco dei file che coincidono nei primi 100 byte con f.
 *
 * Mantiene le opzioni -s (symlink) e -l (hard link) dell'esercizio 1.
 */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>

/* Dimensione del buffer per leggere i file a chunk, evitando di caricare
 * l'intero contenuto in memoria anche per file di grandi dimensioni. */
#define CHUNK_SIZE 4096

/*
 * Confronta il contenuto di path1 e path2.
 *
 * Se is_prefix == 0: confronto esatto — i due file devono essere identici
 *                    in ogni byte (stessa dimensione e stesso contenuto).
 * Se is_prefix == 1: confronto prefisso — i due file devono coincidere
 *                    nei primi prefix_len byte.
 *
 * Ritorna 1 se la condizione è soddisfatta, 0 altrimenti.
 *
 * Ottimizzazione: per il confronto esatto, se le dimensioni differiscono
 * i file non possono essere uguali e si evita di aprirli.
 */
int compare_files(const char *path1, const char *path2, size_t prefix_len, int is_prefix) {
    struct stat st1, st2;

    if (stat(path1, &st1) == -1 || stat(path2, &st2) == -1) return 0;

    /* Se cerchiamo uguaglianza esatta e le dimensioni differiscono, scartiamo subito */
    if (!is_prefix && st1.st_size != st2.st_size) return 0;

    /* Per il confronto esatto, la lunghezza da confrontare è l'intero file */
    if (!is_prefix) prefix_len = st1.st_size;

    FILE *f1 = fopen(path1, "rb");
    if (!f1) return 0;
    FILE *f2 = fopen(path2, "rb");
    if (!f2) { fclose(f1); return 0; }

    char buf1[CHUNK_SIZE];
    char buf2[CHUNK_SIZE];
    size_t bytes_left = prefix_len;
    int match = 1;

    while (bytes_left > 0) {
        size_t to_read = (bytes_left > CHUNK_SIZE) ? CHUNK_SIZE : bytes_left;
        size_t r1 = fread(buf1, 1, to_read, f1);
        size_t r2 = fread(buf2, 1, to_read, f2);

        /* r1 != r2: uno dei due file è finito prima dell'altro nel tratto da confrontare.
         * memcmp: i byte letti sono diversi. In entrambi i casi non c'è match. */
        if (r1 != r2 || memcmp(buf1, buf2, r1) != 0) {
            match = 0;
            break;
        }

        /* EOF raggiunto prima di esaurire prefix_len: entrambi i file sono terminati
         * con lo stesso contenuto fino a qui, consideriamo il confronto soddisfatto. */
        if (r1 < to_read) break;

        bytes_left -= r1;
    }

    fclose(f1);
    fclose(f2);
    return match;
}

/*
 * Visita ricorsivamente il sottoalbero radicato in dir_path e stampa i percorsi
 * degli entry che soddisfano la condizione indicata da option:
 *
 *   's' — symlink il cui target coincide con il file f (stesso inode e device)
 *   'l' — hard link a f (stesso inode e device, non è un symlink)
 *   'c' — file regolare con contenuto identico a f
 *   'p' — file regolare che coincide con f nei primi prefix_len byte
 *
 * target_ino e target_dev identificano univocamente f nel filesystem.
 * target_path serve per aprire f durante il confronto del contenuto.
 *
 * Usa sempre lstat per esaminare le entry: in questo modo non si seguono
 * i symlink e si evitano loop infiniti in presenza di cicli nel grafo del FS.
 */
void visit_dir(const char *dir_path, char option, const char *target_path,
               ino_t target_ino, dev_t target_dev, size_t prefix_len) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Salta . e .. per non risalire l'albero o rientrare nella stessa dir */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat entry_lstat;
        /* lstat ci dà informazioni sull'entry stessa, senza seguire eventuali symlink */
        if (lstat(full_path, &entry_lstat) == -1) {
            perror("lstat");
            continue;
        }

        /* --- Opzione -s: symlink che puntano a f --- */
        if (option == 's' && S_ISLNK(entry_lstat.st_mode)) {
            /* stat segue il symlink e restituisce info sul file puntato:
             * se inode e device coincidono con quelli di f, il link punta a f. */
            struct stat entry_stat;
            if (stat(full_path, &entry_stat) == 0) {
                if (entry_stat.st_ino == target_ino && entry_stat.st_dev == target_dev)
                    printf("%s\n", full_path);
            }
            /* stat fallisce su symlink pendenti (target inesistente): li ignoriamo */
        }
        /* --- Opzione -l: hard link a f --- */
        else if (option == 'l' && !S_ISLNK(entry_lstat.st_mode)) {
            /* I hard link condividono inode e device con il file originale.
             * Escludiamo i symlink perché per loro lstat darebbe l'inode del link stesso. */
            if (entry_lstat.st_ino == target_ino && entry_lstat.st_dev == target_dev)
                printf("%s\n", full_path);
        }
        /* --- Nessuna opzione / -p: confronto contenuto o prefisso --- */
        else if ((option == 'c' || option == 'p') && S_ISREG(entry_lstat.st_mode)) {
            /* Escludiamo f stesso (o eventuali suoi hard link) tramite inode:
             * confrontare f con se stesso sarebbe sempre vero e fuorviante. */
            if (entry_lstat.st_ino != target_ino || entry_lstat.st_dev != target_dev) {
                int is_prefix = (option == 'p');
                if (compare_files(target_path, full_path, prefix_len, is_prefix))
                    printf("%s\n", full_path);
            }
        }

        /* Ricorre nelle sottodirectory; lstat garantisce che non seguiamo
         * symlink a directory, evitando loop su filesystem con cicli. */
        if (S_ISDIR(entry_lstat.st_mode))
            visit_dir(full_path, option, target_path, target_ino, target_dev, prefix_len);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    char   option     = '\0';
    char  *file_path  = NULL;
    char  *dir_path   = NULL;
    size_t prefix_len = 0;

    /* --- Parsing degli argomenti ---
     *
     * Tre forme accettate:
     *   ckfile <file> <dir>            → confronto contenuto esatto
     *   ckfile -s|-l <file> <dir>      → symlink o hard link
     *   ckfile -p <N> <file> <dir>     → confronto primi N byte
     */
    if (argc == 3) {
        option    = 'c';
        file_path = argv[1];
        dir_path  = argv[2];
    } else if (argc == 4) {
        if (strcmp(argv[1], "-s") == 0) option = 's';
        else if (strcmp(argv[1], "-l") == 0) option = 'l';
        else {
            fprintf(stderr, "Invalid option: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        file_path = argv[2];
        dir_path  = argv[3];
    } else if (argc == 5) {
        if (strcmp(argv[1], "-p") == 0) {
            option     = 'p';
            prefix_len = (size_t)atol(argv[2]);
            file_path  = argv[3];
            dir_path   = argv[4];
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <file> <dir>\n",        argv[0]);
        fprintf(stderr, "  %s -s|-l <file> <dir>\n",  argv[0]);
        fprintf(stderr, "  %s -p <N> <file> <dir>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Ottieni inode e device di f: li useremo per identificarlo univocamente
     * nel filesystem durante la visita (confronto hard link e auto-esclusione). */
    struct stat file_stat;
    if (stat(file_path, &file_stat) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    /* Il programma opera solo su file regolari come target */
    if (!S_ISREG(file_stat.st_mode)) {
        fprintf(stderr, "Error: %s is not a regular file\n", file_path);
        exit(EXIT_FAILURE);
    }

    if      (option == 's') printf("Symbolic links pointing to %s:\n", file_path);
    else if (option == 'l') printf("Hard links to %s:\n", file_path);
    else if (option == 'c') printf("Files with exact content as %s:\n", file_path);
    else if (option == 'p') printf("Files sharing the first %zu bytes with %s:\n", prefix_len, file_path);

    visit_dir(dir_path, option, file_path, file_stat.st_ino, file_stat.st_dev, prefix_len);

    return 0;
}
