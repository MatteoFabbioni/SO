/**
 Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 Scrivere due programmi C, tx e rx: tx deve trasferire a rx stringhe di max 8 caratteri usando i valori
 assegnati ai segnali (il parametro value della funzione sigqueue).
 Il programma rx deve per prima cosa stampare il proprio pid e attendere segnali.
 ill programma tx ha due parametri, il pid did rx e il messaggio.
 es: tx 22255 sigmsg
 (posto che rx sia in esecuzione con pid 22255, rx deve stampare sigmsg).
*/

/* _POSIX_C_SOURCE 199309L abilita sigqueue(), siginfo_t e union sigval */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

void sig_handler(int signum, siginfo_t *info, void *context) {
    (void)signum;
    (void)context;
    char message[9] = {0};
    /* il messaggio non e' un puntatore a memoria condivisa: tx ha copiato i
       byte direttamente nel valore di sival_ptr, qui li rileggiamo da li' */
    memcpy(message, &info->si_value.sival_ptr, 8);
    printf("%s\n", message);
}

int main(void) {
    printf("%d\n", getpid());

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = sig_handler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        pause();
    }

    return 0;
}
