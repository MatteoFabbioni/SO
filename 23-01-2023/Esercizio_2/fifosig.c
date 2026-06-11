/**
Esercizio 2: Linguaggio C: 10 punti
Il programma fifosig è una estensione di fifotext. Le righe che riceve attraverso la named pipe sono
composte da due numeri, il pid di un processo e il numero di un segnale. Per ogni riga correttamente
formata il segnale indicato viene mandato al processo indicato dal pid.

In un esempio simile la precedente il comando "echo 12345 15 > /tmp/ff" deve causare l'invio del
segnale 15 al processo 12345.

Scrivere il programma fifosig e un programma di prova che scriva nella pipe il proprio pid e il numero
di SIGUSR1 e controlli di ricevere SIGUSR1.
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<signal.h>
#include<errno.h>

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <fifo_pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *fifo_path = argv[1];

    if(mkfifo(fifo_path, 0666) == -1){
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    int finished = 0;
    while(!finished){
        FILE *fp = fopen(fifo_path, "r");
        if(fp == NULL){
            perror("fopen");
            unlink(fifo_path);
            exit(EXIT_FAILURE);
        }

        char line[4096];
        while(fgets(line, sizeof(line), fp) != NULL){
            size_t len = strlen(line);
            if(len > 0 && line[len - 1] == '\n'){
                line[len - 1] = '\0';
            }

            if(strcmp(line, "FINE") == 0){
                finished = 1;
                break;
            }

            pid_t pid;
            int signum;
            if(sscanf(line, "%d %d", &pid, &signum) == 2){
                if(kill(pid, signum) == -1){
                    perror("kill");
                }
            } else {
                fprintf(stderr, "fifosig: riga non valida: %s\n", line);
            }
        }

        fclose(fp);
    }

    if(unlink(fifo_path) == -1){
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}
