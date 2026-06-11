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
#include <unistd.h>

#define CHUNK_SIZE 8
#define MAX_MSG_LEN 65536

/* Stato globale del trasferimento in corso: l'handler viene invocato una
   volta per ogni segnale (lunghezza, poi un chunk alla volta) e deve
   ricordare a che punto della ricezione si trova fra le varie invocazioni */
static char buffer[MAX_MSG_LEN + 1];
static size_t total_length = 0;
static size_t received_bytes = 0;
static int waiting_length = 1;

void sig_handler(int signum, siginfo_t *info, void *context) {
    (void)signum;
    (void)context;

    if (waiting_length) {
        total_length = (size_t)info->si_value.sival_int;
        received_bytes = 0;
        waiting_length = 0;
    } else {
        size_t remaining = total_length - received_bytes;
        size_t n = remaining < CHUNK_SIZE ? remaining : CHUNK_SIZE;
        memcpy(buffer + received_bytes, &info->si_value.sival_ptr, n);
        received_bytes += n;

        if (received_bytes >= total_length) {
            buffer[total_length] = '\0';
            printf("%s\n", buffer);
            waiting_length = 1;
        }
    }

    /* l'ACK serve a tx per non inviare il chunk successivo prima che questo
       sia stato elaborato: i segnali standard non sono accodati */
    union sigval ack;
    memset(&ack, 0, sizeof(ack));
    sigqueue(info->si_pid, SIGUSR2, ack);
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
