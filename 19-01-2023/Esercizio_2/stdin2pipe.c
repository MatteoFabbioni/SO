/**
 * Esercizio 2: Linguaggio C: 10 punti
 * Estendere il programma stdin2pipe in modo da consentire più di due righe di input.
 *
 * Per esempio, dato il file cmds che contiene 4 righe
 *  ls -l
 *  awk '{print $1}'
 *  sort
 *  uniq
 * l'esecuzione di:
 *  stdin2pipe < cmds
 * sia equivalente al comando:
 *  ls -l | awk '{print $1}' | sort | uniq'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_CMDS 64

static int parse_line(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *token = strtok(line, " \t\n");
    while (token != NULL && argc < max_args - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    return argc;
}

int main(void) {
    char lines[MAX_CMDS][MAX_LINE];
    char *argvs[MAX_CMDS][MAX_ARGS];
    int ncmds = 0;

    while (ncmds < MAX_CMDS && fgets(lines[ncmds], MAX_LINE, stdin) != NULL) {
        if (parse_line(lines[ncmds], argvs[ncmds], MAX_ARGS) == 0) {
            continue; /* riga vuota: non genera un comando */
        }
        ncmds++;
    }

    if (ncmds == 0) {
        fprintf(stderr, "stdin2pipe: nessun comando in input\n");
        exit(EXIT_FAILURE);
    }

    /* serve un'unica pipe tra ogni coppia di comandi consecutivi:
     * pipes[i] collega il comando i al comando i+1 */
    int pipes[MAX_CMDS - 1][2];
    for (int i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pids[MAX_CMDS];

    for (int i = 0; i < ncmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < ncmds - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }

            /* il figlio deve chiudere tutti i fd delle pipe ereditati dal
             * padre, anche quelli non suoi: altrimenti l'ultimo comando
             * della catena non vedrebbe mai EOF in lettura */
            for (int j = 0; j < ncmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(argvs[i][0], argvs[i]);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int status;
    for (int i = 0; i < ncmds; i++) {
        waitpid(pids[i], &status, 0);
    }

    return EXIT_SUCCESS;
}
