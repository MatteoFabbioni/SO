/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * Scrivere il programma stdin2pipe che prenda in input due righe di testo, ogni riga contiene un
 * comando coi rispettivi parametri. Il programma deve lanciare entrambi i comandi in modo tale
 * l'output del primo diventi input del secondo.
 *
 * Per esempio, dato il file cmds:
 *  ls -l
 *  awk '{print $1}'
 * l'esecuzione di:
 *  stdin2pipe < cmds
 * sia equivalente al comando:
 *  ls -l | awk '{print $1}'
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

/* Spezza la riga in argv[] separando sugli spazi: i comandi dell'esempio
 * (ls -l, awk '{print $1}') non contengono spazi dentro gli argomenti, quindi
 * una split semplice e' sufficiente e non serve un parser di quoting di shell. */
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
    char line1[MAX_LINE], line2[MAX_LINE];
    char *argv1[MAX_ARGS], *argv2[MAX_ARGS];

    if (fgets(line1, sizeof(line1), stdin) == NULL) {
        fprintf(stderr, "stdin2pipe: impossibile leggere la prima riga di comando\n");
        exit(EXIT_FAILURE);
    }
    if (fgets(line2, sizeof(line2), stdin) == NULL) {
        fprintf(stderr, "stdin2pipe: impossibile leggere la seconda riga di comando\n");
        exit(EXIT_FAILURE);
    }

    if (parse_line(line1, argv1, MAX_ARGS) == 0) {
        fprintf(stderr, "stdin2pipe: prima riga vuota\n");
        exit(EXIT_FAILURE);
    }
    if (parse_line(line2, argv2, MAX_ARGS) == 0) {
        fprintf(stderr, "stdin2pipe: seconda riga vuota\n");
        exit(EXIT_FAILURE);
    }

    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid1 == 0) {
        /* il primo comando scrive sulla pipe al posto di stdout */
        close(fd[0]);
        if (dup2(fd[1], STDOUT_FILENO) < 0) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd[1]);
        execvp(argv1[0], argv1);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {
        /* il secondo comando legge dalla pipe al posto di stdin */
        close(fd[1]);
        if (dup2(fd[0], STDIN_FILENO) < 0) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(fd[0]);
        execvp(argv2[0], argv2);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    /* il padre non deve scrivere/leggere sulla pipe: va chiusa per evitare
     * che il secondo comando resti in attesa di EOF che non arriva mai */
    close(fd[0]);
    close(fd[1]);

    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    return EXIT_SUCCESS;
}
