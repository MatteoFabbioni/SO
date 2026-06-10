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
/* Abilita le estensioni POSIX e BSD: necessario per sigaction(), write(), pause() */

#include<stdio.h>
/* Fornisce printf() */

#include<stdlib.h>
/* Fornisce EXIT_SUCCESS */

#include<signal.h>
/* Fornisce sigaction(), struct sigaction, SIGUSR1, SIGUSR2 */

#include<unistd.h>
/* Fornisce getpid(), pause(), write(), STDOUT_FILENO */

#include<string.h>
/* Fornisce memset(), strlen() */

#include<sys/types.h>
/* Fornisce pid_t */

void sig_handler(int signum){
    /* Handler condiviso per SIGUSR1 e SIGUSR2: ricostruisce la stringa bit per bit */

    static char message[256] = {0};
    /* Buffer statico per accumulare i caratteri ricevuti; statico perché deve persistere
       tra una chiamata all'handler e l'altra */

    static int bit_index = 0;
    /* Indice del bit corrente all'interno del carattere in costruzione (0-7); statico per persistere */

    static int char_index = 0;
    /* Indice del carattere corrente nel buffer message; statico per persistere */

    if(signum == SIGUSR1 || signum == SIGUSR2){
        /* Verifica che il segnale ricevuto sia uno dei due attesi */

        int bit_value = (signum == SIGUSR2) ? 1 : 0;
        /* Decodifica il segnale: SIGUSR2 → bit 1, SIGUSR1 → bit 0 */

        message[char_index] |= (bit_value << bit_index);
        /* Inserisce il bit ricevuto nella posizione 'bit_index' del carattere corrente:
           shifta il bit_value di 'bit_index' posizioni e lo OR-a nel byte */

        bit_index++;
        /* Avanza all'attesa del prossimo bit */

        if(bit_index == 8){
            /* Sono stati ricevuti tutti gli 8 bit del carattere corrente */

            if(message[char_index] == '\0'){
                /* Il carattere ricevuto è il terminatore '\0': la stringa è completa */

                write(STDOUT_FILENO, message, strlen(message));
                /* Stampa la stringa ricevuta usando write() che è async-signal-safe,
                   a differenza di printf() che non lo è e non va usata nei signal handler */

                write(STDOUT_FILENO, "\n", 1);
                /* Aggiunge il carattere di newline dopo la stringa */

                memset(message, 0, sizeof(message));
                /* Azzera il buffer per poter ricevere la prossima stringa */

                char_index = 0;
                /* Resetta l'indice del carattere per la prossima stringa */

            } else {
                char_index++;
                /* Il carattere non è il terminatore: passa alla ricezione del prossimo carattere */
            }

            bit_index = 0;
            /* Resetta l'indice dei bit per il prossimo carattere da ricevere */
        }
    }
}

int main(int argc, char *argv[]){

    pid_t pid = getpid();
    /* Ottiene il PID del processo corrente */

    printf("semrecv running with pid: %d\n", pid);
    /* Stampa il PID: semsend ne ha bisogno per sapere a chi inviare i segnali */

    struct sigaction sa = {0};
    /* Inizializza a zero la struttura sigaction: sa_mask vuota, nessun flag */

    sa.sa_handler = sig_handler;
    /* Imposta sig_handler come funzione da chiamare alla ricezione del segnale */

    sigaction(SIGUSR1, &sa, NULL);
    /* Registra sig_handler come handler per SIGUSR1 */

    sigaction(SIGUSR2, &sa, NULL);
    /* Registra sig_handler come handler per SIGUSR2 */

    for(;;){
        /* Loop infinito: il processo rimane in attesa di segnali */

        pause();
        /* Sospende il processo fino all'arrivo di un segnale;
           quando il segnale arriva, l'handler viene eseguito e pause() ritorna,
           poi il loop richiama pause() per attendere il segnale successivo */
    }
}
