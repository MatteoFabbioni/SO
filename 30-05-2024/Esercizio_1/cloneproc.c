/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Scrivere un programma cloneproc dato il pid di un processo passato come unico parametro, è in
 * grado di eseguirne una copia. (deve rieseguire lo stesso file con lo stresso argv.
 * consiglio: cercare in /proc/pid/exe e /proc/pid/cmdline le informazioni necessarie (dove pid è il numero
 * di processo.
 * scrivere inoltre un semplice programma che ne dimostri il funzionamento.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_ARGS 256

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *endptr;
    long pid = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || pid <= 0) {
        fprintf(stderr, "Pid non valido: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    char exe_path[64];
    snprintf(exe_path, sizeof(exe_path), "/proc/%ld/exe", pid);

    char resolved_exe[PATH_MAX];
    /* readlink non termina la stringa con '\0': va aggiunto a mano */
    ssize_t n = readlink(exe_path, resolved_exe, sizeof(resolved_exe) - 1);
    if (n < 0) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    resolved_exe[n] = '\0';

    char cmdline_path[64];
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%ld/cmdline", pid);

    FILE *f = fopen(cmdline_path, "r");
    if (f == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* /proc/pid/cmdline contiene gli argomenti originali separati da '\0',
     * ed e' l'unico modo per recuperare il vero argv del processo */
    char buf[4096];
    size_t total = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    if (total == 0) {
        fprintf(stderr, "cmdline vuota per il pid %ld\n", pid);
        exit(EXIT_FAILURE);
    }
    buf[total] = '\0';

    char *new_argv[MAX_ARGS];
    int nargs = 0;
    char *p = buf;
    while ((size_t)(p - buf) < total && nargs < MAX_ARGS - 1) {
        new_argv[nargs++] = p;
        p += strlen(p) + 1;
    }
    new_argv[nargs] = NULL;

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        execv(resolved_exe, new_argv);
        perror("execv");
        exit(EXIT_FAILURE);
    }

    int status;
    if (waitpid(child, &status, 0) < 0) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return EXIT_FAILURE;
}
