/**
modificare il  programma semrecv in modo che posa ricevere
contemporaneamente messaggi da molteplici mittenti diversi. (rallentare ad arte semsend in modo
che a semrecv arrivino i bit dei caratteri di più mittenti  inframezzati)
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX e BSD: necessario per sigaction(), write(), pause() */

#include <stdio.h>
/* Fornisce printf() */

#include <stdlib.h>
/* Fornisce EXIT_SUCCESS */

#include <signal.h>
/* Fornisce sigaction(), struct sigaction, siginfo_t, SIGUSR1, SIGUSR2, SA_SIGINFO */

#include <unistd.h>
/* Fornisce getpid(), pause(), write(), STDOUT_FILENO */

#include <string.h>
/* Fornisce memset(), strlen() */

#include <sys/types.h>
/* Fornisce pid_t */

#define MAX_SENDERS 16
/* Numero massimo di mittenti gestibili contemporaneamente */

typedef struct {
    pid_t pid;         /* PID del mittente; 0 indica uno slot libero */
    char message[256]; /* Buffer per accumulare i caratteri del messaggio in costruzione */
    int bit_index;     /* Indice del bit corrente nel carattere (0-7) */
    int char_index;    /* Indice del carattere corrente nel buffer message */
} SenderState;
/* Struttura che mantiene lo stato di ricezione separato per ogni mittente */

static SenderState senders[MAX_SENDERS];
/* Array di stati: un elemento per ogni mittente attivo contemporaneamente */

SenderState *get_sender(pid_t pid){
    /* Cerca l'entry del mittente con PID 'pid'; se non esiste ne crea una nuova */

    for(int i = 0; i < MAX_SENDERS; i++){
        if(senders[i].pid == pid)
            return &senders[i];
        /* Slot già esistente per questo mittente: lo restituisce */
    }

    for(int i = 0; i < MAX_SENDERS; i++){
        if(senders[i].pid == 0){
            /* Slot libero trovato: lo inizializza per il nuovo mittente */
            senders[i].pid = pid;
            senders[i].bit_index = 0;
            senders[i].char_index = 0;
            memset(senders[i].message, 0, sizeof(senders[i].message));
            return &senders[i];
        }
    }

    return NULL;
    /* Nessuno slot disponibile: troppi mittenti contemporanei */
}

void sig_handler(int signum, siginfo_t *info, void *context){
    /* Handler per SIGUSR1 e SIGUSR2: ricostruisce il messaggio bit per bit,
       mantenendo uno stato separato per ogni mittente tramite info->si_pid */

    pid_t sender_pid = info->si_pid;
    /* PID del processo che ha inviato il segnale: disponibile grazie a SA_SIGINFO */

    SenderState *s = get_sender(sender_pid);
    /* Recupera (o crea) lo stato di ricezione per questo mittente */

    if(s == NULL) return;
    /* Troppi mittenti attivi: ignora il segnale */

    int bit_value = (signum == SIGUSR2) ? 1 : 0;
    /* Decodifica il segnale: SIGUSR2 → bit 1, SIGUSR1 → bit 0 */

    s->message[s->char_index] |= (bit_value << s->bit_index);
    /* Inserisce il bit ricevuto nella posizione 'bit_index' del carattere corrente */

    s->bit_index++;
    /* Avanza al prossimo bit */

    if(s->bit_index == 8){
        /* Sono stati ricevuti tutti gli 8 bit del carattere corrente */

        if(s->message[s->char_index] == '\0'){
            /* Carattere terminatore '\0' ricevuto: il messaggio è completo */

            write(STDOUT_FILENO, s->message, strlen(s->message));
            /* Stampa il messaggio usando write(), che è async-signal-safe */

            write(STDOUT_FILENO, "\n", 1);
            /* Aggiunge il newline finale */

            s->pid = 0;
            /* Libera lo slot: questo mittente ha terminato la trasmissione */

            memset(s->message, 0, sizeof(s->message));
            /* Azzera il buffer per un eventuale riutilizzo dello slot */

        } else {
            s->char_index++;
            /* Carattere normale: passa al prossimo */
        }

        s->bit_index = 0;
        /* Resetta l'indice dei bit per il prossimo carattere */
    }
}

int main(int argc, char *argv[]){

    memset(senders, 0, sizeof(senders));
    /* Inizializza tutti gli slot a zero (pid=0 → libero) */

    pid_t pid = getpid();
    /* Ottiene il PID del processo corrente */

    printf("semrecv running with pid: %d\n", pid);
    /* Stampa il PID: ogni semsend ne ha bisogno per sapere a chi inviare i segnali */

    struct sigaction sa = {0};
    /* Inizializza a zero la struttura sigaction */

    sa.sa_sigaction = sig_handler;
    /* Usa sa_sigaction invece di sa_handler per ricevere siginfo_t con il PID del mittente */

    sa.sa_flags = SA_SIGINFO;
    /* SA_SIGINFO: abilita il passaggio di siginfo_t all'handler,
       rendendo disponibile info->si_pid (PID del mittente) */

    sigaction(SIGUSR1, &sa, NULL);
    /* Registra sig_handler come handler per SIGUSR1 */

    sigaction(SIGUSR2, &sa, NULL);
    /* Registra sig_handler come handler per SIGUSR2 */

    for(;;){
        /* Loop infinito: il processo rimane in attesa di segnali */

        pause();
        /* Sospende il processo fino all'arrivo di un segnale */
    }
}
