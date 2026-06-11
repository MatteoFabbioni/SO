/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti.
 * Usando il metodo inotify implementare il programma dircat.
 * dircat ha 2 parametri: il pathname di una directory (vuota) D e di un file F.
 * Ogni volta che un file viene messo in D il programma dircat aggiunge a F una riga di testata
 * contenente il nome del file e ne copia il contenuto in F. Finita la copia il file che era stato messo in D
 * viene cancellato (la directory D torna vuota).
 *
 * (per fare gli esperimenti si consiglia di preparare i file di prova in un'altra directory e copiarli in D).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

static void append_header(int out_fd, const char *name) {
    char header[PATH_MAX + 16];
    int n = snprintf(header, sizeof(header), "=== %s ===\n", name);
    if (write(out_fd, header, n) == -1)
        perror("write");
}

static void append_contents(int out_fd, const char *path) {
    int in_fd = open(path, O_RDONLY);
    if (in_fd == -1) {
        perror("open");
        return;
    }

    char buf[4096];
    ssize_t r;
    while ((r = read(in_fd, buf, sizeof(buf))) > 0) {
        if (write(out_fd, buf, r) == -1) {
            perror("write");
            break;
        }
    }
    if (r == -1)
        perror("read");

    close(in_fd);
}

static void handle_new_file(const char *dir, const char *name, int out_fd) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    append_header(out_fd, name);
    append_contents(out_fd, path);

    if (unlink(path) == -1)
        perror("unlink");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <directory D> <file F>\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return EXIT_FAILURE;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s: non e' una directory\n", argv[1]);
        return EXIT_FAILURE;
    }

    /* F viene aperto una sola volta in append, cosi' ogni copia successiva
     * si accoda senza ritoccare quanto scritto in precedenza. */
    int out_fd = open(argv[2], O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (out_fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify_init");
        close(out_fd);
        return EXIT_FAILURE;
    }

    /* IN_CLOSE_WRITE/IN_MOVED_TO: aspettiamo che il file sia completo
     * (sia che venga scritto, sia che venga spostato/copiato in D). */
    if (inotify_add_watch(inotify_fd, argv[1], IN_CLOSE_WRITE | IN_MOVED_TO) == -1) {
        perror("inotify_add_watch");
        close(inotify_fd);
        close(out_fd);
        return EXIT_FAILURE;
    }

    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));

    for (;;) {
        ssize_t len = read(inotify_fd, buf, sizeof(buf));
        if (len == -1) {
            if (errno == EINTR)
                continue;
            perror("read");
            break;
        }

        ssize_t offset = 0;
        while (offset < len) {
            struct inotify_event *event = (struct inotify_event *) &buf[offset];
            if (event->len > 0)
                handle_new_file(argv[1], event->name, out_fd);
            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    close(out_fd);
    return EXIT_SUCCESS;
}
