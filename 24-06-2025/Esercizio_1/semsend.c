/**
 Scrivere due programmi C: semsend e semrecv.
Il programma  semrecv  stampa il proprio  pid  e si pone in attesa. Il programma  semsend  ha due
parametri: il pid del processo semrecv e una stringa.
La   stringa   passata   come   ultimo   parametro   a  semsend  deve   essere   trasferita   a  semrecv  e   da
quest'ultimo stampata.
Ogni carattere della stringa (incluso il terminatore) deve essere spedito bit a bit usando i segnali
SIGUSR1 e SIGUSR2.
*/

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX e BSD: necessario per sigaction(), kill(), usleep() */

#include<stdio.h>
/* Fornisce printf(), perror() */

#include<stdlib.h>
/* Fornisce exit(), atoi() */

#include<signal.h>
/* Fornisce kill(), SIGUSR1, SIGUSR2 */

#include<unistd.h>
/* Fornisce usleep() */

#include<string.h>
/* Fornisce strlen() */

#include<sys/types.h>
/* Fornisce pid_t */

void semsend(pid_t pid, char *message){
    /* Invia la stringa 'message' al processo 'pid' bit per bit tramite segnali */

    for(size_t i = 0; i < strlen(message) + 1; i++){
        /* Itera su ogni carattere della stringa, incluso il terminatore '\0' (+1) */

        char c = message[i];
        /* Carattere corrente da trasmettere */

        for(int bit = 0; bit < 8; bit++){
            /* Itera su tutti gli 8 bit del carattere, dal meno significativo (LSB) al più significativo (MSB) */

            int bit_value = (c >> bit) & 1;
            /* Estrae il valore del bit in posizione 'bit':
               shifta 'c' a destra di 'bit' posizioni e maschera con 1 per isolare il bit */

            int signal_to_send = (bit_value == 0) ? SIGUSR1 : SIGUSR2;
            /* SIGUSR1 codifica il bit 0, SIGUSR2 codifica il bit 1 */

            if(kill(pid, signal_to_send) == -1){
                /* kill() invia il segnale al processo con PID 'pid'; restituisce -1 in caso di errore */

                perror("Error sending signal");
                exit(-1);
                /* Termina il programma in caso di errore nell'invio del segnale */
            }

            usleep(100000);
            /* Attende 100ms prima di inviare il prossimo bit:
               SIGUSR1/SIGUSR2 non sono accodati, segnali inviati troppo velocemente
               potrebbero sovrapporsi e andare persi nel ricevente */
        }
    }
}

int main(int argc, char *argv[]){

    if(argc != 3){
        printf("Usage: %s <pid> <message>\n", argv[0]);
        return -1;
        /* Il programma richiede esattamente due argomenti: PID del ricevente e stringa da inviare */
    }

    pid_t pid = atoi(argv[1]);
    /* Converte il primo argomento (stringa) nel PID numerico del processo ricevente */

    char *message = argv[2];
    /* Il secondo argomento è la stringa da trasmettere bit per bit */

    semsend(pid, message);
    /* Avvia la trasmissione bit per bit della stringa verso il processo ricevente */

    return 0;
    /* Termina il programma con successo */
}
