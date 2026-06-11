/**
 * Esercizio 2: Linguaggio C: 10 punti
 * modificare il programma dell'esercizio 1 per fare in modo che execname2 scriva l'output
 * dell'esecuzione nel file invece che cancellarlo.
 * Nell'esempio precedente il programma execname2 non emette alcun output ma il comando
 *  cat 'exec/echo ciao mare'
 * produce come risultato:
 *  ciao mare
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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

static void run_into_file(const char *dir, const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    char name_copy[PATH_MAX];
    strncpy(name_copy, name, sizeof(name_copy) - 1);
    name_copy[sizeof(name_copy) - 1] = '\0';

    char *args[MAX_ARGS];
    if (split_args(name_copy, args, MAX_ARGS) == 0)
        return;

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        /* O_TRUNC: il file esiste gia' (e' stato creato da touch/redirezione),
         * lo svuotiamo per scriverci solo l'output del comando. */
        int fd = open(path, O_WRONLY | O_TRUNC);
        if (fd == -1) {
            perror("open");
            _exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(EXIT_FAILURE);
        }
        close(fd);

        execvp(args[0], args);
        perror("execvp");
        _exit(EXIT_FAILURE);
    }

    if (waitpid(pid, NULL, 0) == -1)
        perror("waitpid");
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
                run_into_file(argv[1], event->name);
            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    close(inotify_fd);
    return EXIT_SUCCESS;
}
