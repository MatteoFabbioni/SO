/**
 Esercizio 2: Linguaggio C: 10 punti
 Estendere i programmi dell'es.1 per consentire il trasferimento di stringhe di lunghezza arbitraria
 (iterando il procedimento 8 caratteri alla volta).
*/

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>

#define CHUNK_SIZE 8

/* Settato dall'handler quando arriva l'ACK di rx (SIGUSR2).
   sig_atomic_t e volatile: lettura/scrittura sicura tra main e signal handler */
static volatile sig_atomic_t ack_received = 0;

void ack_handler(int signum) {
    (void)signum;
    ack_received = 1;
}

/* sigsuspend invece di pause(): installa atomicamente la maschera che sblocca
   solo SIGUSR2, evitando la race fra il controllo del flag e l'attesa del segnale */
void wait_ack(void) {
    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGUSR2);
    while (!ack_received) {
        sigsuspend(&mask);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <pid_rx> <message>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    sa.sa_handler = ack_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR2, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    pid_t pid = atoi(argv[1]);
    char *message = argv[2];
    size_t len = strlen(message);

    /* la lunghezza viaggia come primo segnale, cosi' rx sa quanti chunk attendersi */
    union sigval len_value;
    len_value.sival_int = (int)len;
    ack_received = 0;
    if (sigqueue(pid, SIGUSR1, len_value) == -1) {
        perror("sigqueue");
        exit(EXIT_FAILURE);
    }
    wait_ack();

    for (size_t sent = 0; sent < len; sent += CHUNK_SIZE) {
        size_t n = len - sent < CHUNK_SIZE ? len - sent : CHUNK_SIZE;

        union sigval value;
        memset(&value, 0, sizeof(value));
        memcpy(&value.sival_ptr, message + sent, n);

        ack_received = 0;
        if (sigqueue(pid, SIGUSR1, value) == -1) {
            perror("sigqueue");
            exit(EXIT_FAILURE);
        }
        wait_ack();
    }

    return 0;
}
