/**
 * Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 * scrivere inoltre un semplice programma che ne dimostri il funzionamento.
 *
 * demo_cloneproc avvia un processo "sleep 30" come bersaglio, ne stampa il pid
 * e poi invoca ./cloneproc su quel pid per mostrare che viene rieseguito lo
 * stesso programma con lo stesso argv.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main(void) {
    pid_t target = fork();
    if (target < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (target == 0) {
        execlp("sleep", "sleep", "30", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    printf("Processo bersaglio avviato: sleep 30, pid=%d\n", target);

    char pid_str[16];
    snprintf(pid_str, sizeof(pid_str), "%d", target);

    pid_t cloner = fork();
    if (cloner < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cloner == 0) {
        execl("./cloneproc", "./cloneproc", pid_str, (char *)NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    int status;
    waitpid(cloner, &status, 0);
    printf("cloneproc terminato (la copia di sleep 30 e' ora in esecuzione)\n");

    /* il bersaglio originale resta vivo: lo terminiamo per non lasciare
     * processi orfani al termine della demo */
    kill(target, SIGTERM);
    waitpid(target, &status, 0);

    return 0;
}
