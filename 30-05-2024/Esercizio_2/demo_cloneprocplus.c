/**
 * Esercizio 2: Linguaggio C: 10 punti
 * scrivere inoltre un semplice programma che ne dimostri il funzionamento.
 *
 * demo_cloneprocplus avvia un processo bersaglio (sh -c 'pwd; echo $DEMO_VAR; sleep 30')
 * con una directory corrente diversa da quella del demo e una variabile
 * d'ambiente DEMO_VAR impostata, poi invoca ./cloneprocplus sul suo pid per
 * mostrare che la copia eredita sia la cwd sia l'ambiente del processo originale.
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
        if (chdir("/tmp") < 0) {
            perror("chdir");
            exit(EXIT_FAILURE);
        }
        if (setenv("DEMO_VAR", "valore_di_prova", 1) < 0) {
            perror("setenv");
            exit(EXIT_FAILURE);
        }
        execlp("sh", "sh", "-c", "echo cwd=$(pwd); echo DEMO_VAR=$DEMO_VAR; sleep 30", (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    }

    printf("Processo bersaglio avviato in /tmp con DEMO_VAR impostata, pid=%d\n", target);
    sleep(1);

    char pid_str[16];
    snprintf(pid_str, sizeof(pid_str), "%d", target);

    pid_t cloner = fork();
    if (cloner < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cloner == 0) {
        execl("./cloneprocplus", "./cloneprocplus", pid_str, (char *)NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    }

    int status;
    waitpid(cloner, &status, 0);

    kill(target, SIGTERM);
    waitpid(target, &status, 0);

    return 0;
}
