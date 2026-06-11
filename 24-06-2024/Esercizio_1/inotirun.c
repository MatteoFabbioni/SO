/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Usando inotify scrivere un programma inotirun che ha come parametro il pathname di una
 * directory vuota che chiameremo D. Quando vengono inseriti file in D questi vengono eseguiti (uno
 * alla volta) e cancellati. I file in D hanno il seguente formato:
 * * il pathname dell'eseguibile
 * * una riga per ogni elemento di argv.
 * Es:
 * /bin/ls
 * ls
 * -l
 * /tmp
 */

#define _DEFAULT_SOURCE
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

/* legge il file riga per riga: la prima riga e' il pathname dell'eseguibile,
 * le righe successive sono gli elementi di argv (incluso argv[0]) */
static int read_command(FILE *f, char *exec_path, size_t exec_size, char *args[], char arg_buf[][PATH_MAX], int max_args) {
    if (fgets(exec_path, exec_size, f) == NULL)
        return -1;
    exec_path[strcspn(exec_path, "\n")] = '\0';

    int argc = 0;
    while (argc < max_args - 1 && fgets(arg_buf[argc], PATH_MAX, f) != NULL) {
        arg_buf[argc][strcspn(arg_buf[argc], "\n")] = '\0';
        args[argc] = arg_buf[argc];
        argc++;
    }
    args[argc] = NULL;
    return argc;
}

static void run_and_remove(const char *dir, const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return;
    }

    char exec_path[PATH_MAX];
    char arg_buf[MAX_ARGS][PATH_MAX];
    char *args[MAX_ARGS];
    int n = read_command(f, exec_path, sizeof(exec_path), args, arg_buf, MAX_ARGS);
    fclose(f);

    if (n <= 0) {
        fprintf(stderr, "%s: formato non valido\n", path);
    } else {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
        } else if (pid == 0) {
            execv(exec_path, args);
            perror("execv");
            _exit(EXIT_FAILURE);
        } else {
            if (waitpid(pid, NULL, 0) == -1)
                perror("waitpid");
        }
    }

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

    /* CLOSE_WRITE garantisce che il file sia stato scritto per intero
     * prima che venga letto ed eseguito */
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
