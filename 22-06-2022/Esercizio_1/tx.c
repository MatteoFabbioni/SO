/**
 Esercizio 1: Linguaggio C (obbligatorio) 20 punti
 Scrivere due programmi C, tx e rx: tx deve trasferire a rx stringhe di max 8 caratteri usando i valori
 assegnati ai segnali (il parametro value della funzione sigqueue).
 Il programma rx deve per prima cosa stampare il proprio pid e attendere segnali.
 ill programma tx ha due parametri, il pid did rx e il messaggio.
 es: tx 22255 sigmsg
 (posto che rx sia in esecuzione con pid 22255, rx deve stampare sigmsg).
*/

/* _POSIX_C_SOURCE 199309L abilita sigqueue() e union sigval, non disponibili nel C standard */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid_rx> <message (max 8 chars)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    pid_t pid = atoi(argv[1]);
    char *message = argv[2];

    if (strlen(message) > 8) {
        fprintf(stderr, "Error: message must be at most 8 characters long\n");
        exit(EXIT_FAILURE);
    }

    union sigval value;
    /* sival_ptr e' 8 byte su sistemi a 64 bit: ci copiamo dentro i byte del
       messaggio usandolo come contenitore di dati, non come puntatore valido */
    memset(&value, 0, sizeof(value));
    memcpy(&value.sival_ptr, message, strlen(message));

    if (sigqueue(pid, SIGUSR1, value) == -1) {
        perror("sigqueue");
        exit(EXIT_FAILURE);
    }

    return 0;
}
