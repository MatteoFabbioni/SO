/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Creare una directory chiamata exec. Scrivere un programma execname che se viene aggiunto un file
 * nela directory exec interpreti il nome del file come comando con parametri, lo esegua e cancelli il file.
 * es: sopo aver lanciato execname:
 *  execname exec
 * a seguito di questo comando:
 *  touch 'exec/echo ciao mare'
 * il programma stampa:
 *  ciao mare
 * (consiglio, usare inotify)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#define MAX_ARGS 64

/* split modifica la stringa sul posto inserendo '\0' dove trova spazi:
 * va bene perche' il buffer name e' una copia locale usata una sola volta. */
static int split_args(char *name, char *args[], int max_args) {
    int argc = 0;
    char *token = strtok(name, " ");
    while (token != NULL && argc < max_args - 1) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;
    return argc;
}

static void run_and_remove(const char *dir, const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    char name_copy[PATH_MAX];
    strncpy(name_copy, name, sizeof(name_copy) - 1);
    name_copy[sizeof(name_copy) - 1] = '\0';

    char *args[MAX_ARGS];
    if (split_args(name_copy, args, MAX_ARGS) == 0) {
        if (unlink(path) == -1)
            perror("unlink");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execvp(args[0], args);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    if (waitpid(pid, NULL, 0) == -1)
        perror("waitpid");

    if (unlink(path) == -1)
        perror("unlink");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory>\n", argv[0]);
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

    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("inotify_init");
        return EXIT_FAILURE;
    }

    /* CLOSE_WRITE: touch genera CREATE+CLOSE_WRITE; aspettiamo CLOSE_WRITE
     * cosi' il file e' completo (utile anche se qualcuno ci scrive dentro). */
    if (inotify_add_watch(inotify_fd, argv[1], IN_CLOSE_WRITE) == -1) {
        perror("inotify_add_watch");
        close(inotify_fd);
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
                run_and_remove(argv[1], event->name);
            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    return EXIT_SUCCESS;
}
