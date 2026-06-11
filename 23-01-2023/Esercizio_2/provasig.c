/**
Esercizio 2: Linguaggio C: 10 punti
Programma di prova per fifosig: scrive nella pipe il proprio pid e il numero di SIGUSR1 e controlla
di ricevere SIGUSR1.

Esempio d'uso:
fifosig /tmp/ff &
provasig /tmp/ff
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<errno.h>

static volatile sig_atomic_t got_sigusr1 = 0;

static void handler(int signo){
    (void)signo;
    got_sigusr1 = 1;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <fifo_pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *fifo_path = argv[1];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    if(sigaction(SIGUSR1, &sa, NULL) == -1){
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(fifo_path, "w");
    if(fp == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "%d %d\n", getpid(), SIGUSR1);
    fclose(fp);

    /* pause si risveglia non appena arriva un segnale: il controllo sul flag
     * serve a distinguere SIGUSR1 da eventuali altri segnali ricevuti */
    while(!got_sigusr1){
        pause();
    }

    printf("provasig: ricevuto SIGUSR1\n");

    return 0;
}
