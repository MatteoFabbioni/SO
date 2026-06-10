/* Abilita le estensioni POSIX.1b: senza questo define, sigqueue(), siginfo_t
 * e union sigval non sono disponibili perché non fanno parte del C standard. */
#define _POSIX_C_SOURCE 199309L

#include<stdio.h>      /* printf */
#include<stdlib.h>     /* exit */
#include<signal.h>     /* sigaction, sigqueue, SIGUSR1, SIGUSR2, siginfo_t, union sigval */
#include<unistd.h>     /* getpid, pause */
#include<string.h>     /* memcpy, memset */
#include<sys/types.h>  /* pid_t */

/* Stato globale del trasferimento in corso.
 * Globale perché l'handler viene invocato più volte (una per chunk)
 * e deve ricordare il contesto tra una chiamata e l'altra. */
static char buffer[1024] = {0}; /* accumula i chunk ricevuti fino al messaggio completo */
static int total_length = 0;    /* lunghezza totale attesa, ricevuta nel primo segnale */
static int received_bytes = 0;  /* byte effettivamente ricevuti finora */
static int waiting_length = 1;  /* macchina a stati: 1 = aspetta lunghezza, 0 = aspetta chunk */

/* Handler invocato ad ogni SIGUSR1.
 *
 * Prima invocazione (waiting_length == 1):
 *   sival_int contiene la lunghezza totale della stringa.
 * Invocazioni successive (waiting_length == 0):
 *   sival_ptr contiene un chunk da 8 byte (i byte della stringa packed nel valore del puntatore).
 *
 * Dopo ogni ricezione invia SIGUSR2 come ACK al mittente (info->si_pid). */
void sig_handler(int signum, siginfo_t *info, void *context){
    union sigval ack = {0};     /* payload dell'ACK: contenuto irrilevante, conta solo il segnale */

    if(waiting_length){
        /* ── FASE 1: primo segnale, contiene la lunghezza totale ── */
        total_length = info->si_value.sival_int;  /* legge la lunghezza da sival_int */
        waiting_length = 0;                        /* passa alla fase di ricezione chunk */
    } else {
        /* ── FASE 2: segnali successivi, contengono chunk da 8 byte ── */
        int remaining = total_length - received_bytes;        /* byte ancora da ricevere */
        int chunk_size = remaining < 8 ? remaining : 8;      /* l'ultimo chunk può essere < 8 byte */
        memcpy(buffer + received_bytes,                       /* copia nella posizione corretta del buffer */
               &info->si_value.sival_ptr,                    /* estrae i byte dal valore del puntatore */
               chunk_size);
        received_bytes += chunk_size;                         /* aggiorna il contatore dei byte ricevuti */

        if(received_bytes >= total_length){
            /* messaggio completo: stampa e resetta lo stato per una nuova trasmissione */
            buffer[total_length] = '\0';          /* aggiunge terminatore di stringa */
            printf("Received message: %s\n", buffer);
            memset(buffer, 0, sizeof(buffer));    /* pulisce il buffer per il prossimo messaggio */
            total_length = 0;                     /* resetta la lunghezza attesa */
            received_bytes = 0;                   /* resetta il contatore byte */
            waiting_length = 1;                   /* torna ad aspettare una nuova lunghezza */
        }
    }

    /* Invia ACK a sigtx dopo ogni segnale (sia lunghezza che chunk).
     * info->si_pid è il PID del mittente: il kernel lo valorizza automaticamente
     * in siginfo_t quando il segnale è stato inviato con sigqueue(). */
    sigqueue(info->si_pid, SIGUSR2, ack);
}

void sigrx(){
    pid_t pid = getpid();                          /* recupera il proprio PID */
    printf("sigrx running with pid: %d\n", pid);  /* lo stampa così sigtx sa a chi inviare */

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;      /* handler esteso: riceve siginfo_t con si_pid e si_value */
    sa.sa_sigaction = sig_handler; /* punta all'handler con firma void(int, siginfo_t*, void*) */
    sigemptyset(&sa.sa_mask);      /* nessun segnale aggiuntivo bloccato durante l'handler */
    sigaction(SIGUSR1, &sa, NULL); /* installa l'handler per SIGUSR1 (dati in arrivo da sigtx) */

    for(;;){
        pause(); /* sospende il processo fino al prossimo segnale: non spreca CPU */
    }
    exit(0); /* irraggiungibile, ma documenta l'intenzione di uscire solo su segnale esterno */
}

int main(){
    sigrx(); /* sigrx non ha argomenti: il PID lo comunica lui stesso stampandolo */
    return 0;
}
