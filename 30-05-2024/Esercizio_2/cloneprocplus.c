/**
 * Esercizio 2: Linguaggio C: 10 punti
 * Scrivere una estensione del programma dell'esercizio1 cloneproc+ in grado di clonare anche la
 * directory corrente e l'ambiente (environment) .
 * scrivere inoltre un semplice programma che ne dimostri il funzionamento.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>

#define MAX_ARGS 256
#define MAX_ENV 1024

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
    ssize_t n = readlink(exe_path, resolved_exe, sizeof(resolved_exe) - 1);
    if (n < 0) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    resolved_exe[n] = '\0';

    char cwd_path[64];
    snprintf(cwd_path, sizeof(cwd_path), "/proc/%ld/cwd", pid);

    char resolved_cwd[PATH_MAX];
    n = readlink(cwd_path, resolved_cwd, sizeof(resolved_cwd) - 1);
    if (n < 0) {
        perror("readlink");
        exit(EXIT_FAILURE);
    }
    resolved_cwd[n] = '\0';

    char cmdline_path[64];
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%ld/cmdline", pid);

    FILE *f = fopen(cmdline_path, "r");
    if (f == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char argbuf[4096];
    size_t arg_total = fread(argbuf, 1, sizeof(argbuf) - 1, f);
    fclose(f);

    if (arg_total == 0) {
        fprintf(stderr, "cmdline vuota per il pid %ld\n", pid);
        exit(EXIT_FAILURE);
    }
    argbuf[arg_total] = '\0';

    char *new_argv[MAX_ARGS];
    int nargs = 0;
    char *p = argbuf;
    while ((size_t)(p - argbuf) < arg_total && nargs < MAX_ARGS - 1) {
        new_argv[nargs++] = p;
        p += strlen(p) + 1;
    }
    new_argv[nargs] = NULL;

    char environ_path[64];
    snprintf(environ_path, sizeof(environ_path), "/proc/%ld/environ", pid);

    f = fopen(environ_path, "r");
    if (f == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* buffer statico: deve restare vivo fino all'execve, non puo' essere
     * una variabile locale liberata al ritorno della funzione */
    static char envbuf[65536];
    size_t env_total = fread(envbuf, 1, sizeof(envbuf) - 1, f);
    fclose(f);
    envbuf[env_total] = '\0';

    char *new_envp[MAX_ENV];
    int nenv = 0;
    p = envbuf;
    while ((size_t)(p - envbuf) < env_total && nenv < MAX_ENV - 1) {
        new_envp[nenv++] = p;
        p += strlen(p) + 1;
    }
    new_envp[nenv] = NULL;

    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        if (chdir(resolved_cwd) < 0) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
        execve(resolved_exe, new_argv, new_envp);
        perror("execve");
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
