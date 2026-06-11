/**
 * Esercizio 2: Linguaggio C: 10 punti
 * Estendere l'esercizio 1 creando il programma inotimrun che consenta per ogni file in D l'esecuzione
 * sequenziale di più programmi: I file da inserire in D possono contenere più comandi separati da una
 * riga vuota.
 * Es:
 * /bin/ls
 * ls
 * /rmp
 *
 * /bin/cat
 * cat
 * /etc/hostname
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

/* legge un singolo comando: prima riga il pathname dell'eseguibile, righe
 * successive gli elementi di argv, fino a riga vuota o fine file.
 * Ritorna il numero di argv letti, 0 se non c'e' nessun altro comando (EOF
 * immediato), -1 in caso di formato non valido (comando senza argv). */
static int read_command(FILE *f, char *exec_path, size_t exec_size, char *args[], char arg_buf[][PATH_MAX], int max_args) {
    if (fgets(exec_path, exec_size, f) == NULL)
        return 0;
    exec_path[strcspn(exec_path, "\n")] = '\0';

    int argc = 0;
    while (argc < max_args - 1 && fgets(arg_buf[argc], PATH_MAX, f) != NULL) {
        arg_buf[argc][strcspn(arg_buf[argc], "\n")] = '\0';
        if (arg_buf[argc][0] == '\0')
            break;
        args[argc] = arg_buf[argc];
        argc++;
    }
    args[argc] = NULL;
    return argc == 0 ? -1 : argc;
}

static void run_command(const char *exec_path, char *args[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        execv(exec_path, args);
        perror("execv");
        _exit(EXIT_FAILURE);
    }
    if (waitpid(pid, NULL, 0) == -1)
        perror("waitpid");
}

static void run_and_remove(const char *dir, const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror("fopen");
        return;
    }

    for (;;) {
        char exec_path[PATH_MAX];
        char arg_buf[MAX_ARGS][PATH_MAX];
        char *args[MAX_ARGS];
        int n = read_command(f, exec_path, sizeof(exec_path), args, arg_buf, MAX_ARGS);

        if (n == 0)
            break;
        if (n < 0) {
            fprintf(stderr, "%s: formato non valido\n", path);
            break;
        }
        /* esecuzione sequenziale: aspettiamo il termine di un comando
         * prima di leggere ed eseguire il successivo */
        run_command(exec_path, args);
    }

    fclose(f);

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
