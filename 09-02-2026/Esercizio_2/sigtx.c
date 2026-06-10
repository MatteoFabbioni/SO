/* Abilita le estensioni POSIX.1b: senza questo define, sigqueue(), siginfo_t
 * e union sigval non sono disponibili perché non fanno parte del C standard. */
#define _POSIX_C_SOURCE 199309L

#include<stdio.h>      /* printf, perror */
#include<stdlib.h>     /* atoi, exit */
#include<signal.h>     /* sigaction, sigqueue, sigsuspend, SIGUSR1, SIGUSR2, union sigval */
#include<unistd.h>     /* getpid */
#include<string.h>     /* strlen, memset, memcpy */
#include<sys/types.h>  /* pid_t */

/* Restituisce il minore tra a e b.
 * Serve per calcolare quanti byte copiare nell'ultimo chunk (può essere < 8). */
#define MIN(a,b) ((a)<(b)?(a):(b))

/* Flag settato dall'handler quando arriva l'ACK (SIGUSR2).
 * volatile: impedisce al compilatore di ottimizzare via la variabile.
 * sig_atomic_t: tipo garantito leggibile/scrivibile atomicamente nei signal handler. */
volatile sig_atomic_t ack_received = 0;

/* Handler per SIGUSR2: sigrx invia questo segnale come conferma (ACK) dopo ogni chunk.
 * Non fa nulla tranne alzare il flag: printf non è async-signal-safe e non va usato qui. */
void ack_handler(int signum){
    ack_received = 1; /* notifica al main loop che l'ACK è arrivato */
}

/* Invia 'message' al processo 'pid' chunk per chunk (8 byte alla volta).
 *
 * Protocollo:
 *   1. invia la lunghezza totale come sival_int  →  aspetta ACK
 *   2. per ogni chunk: copia 8 byte in sival_ptr  →  invia  →  aspetta ACK
 *
 * Il trucco su 64 bit: void* è 8 byte = dimensione di un chunk.
 * I byte della stringa vengono copiati nel VALORE di sival_ptr (non come
 * puntatore), così il kernel li trasmette senza condividere memoria tra processi. */
void sigtx(pid_t pid, char *message){
    int len = strlen(message);                                  /* lunghezza della stringa senza '\0' */
    int chunks = (len / 8) + (len % 8 != 0 ? 1 : 0);          /* numero di chunk: arrotonda per eccesso (es. 9 byte → 2 chunk) */

    /* Maschera per sigsuspend: blocca tutti i segnali tranne SIGUSR2.
     * sigsuspend installa questa maschera atomicamente prima di sospendere,
     * evitando la race condition che si avrebbe con pause(). */
    sigset_t mask;
    sigfillset(&mask);           /* inizia bloccando tutti i segnali */
    sigdelset(&mask, SIGUSR2);   /* sblocca solo SIGUSR2, l'ACK atteso */

    /* ── FASE 1: invia la lunghezza totale ──────────────────────────────── */
    union sigval val_len;
    val_len.sival_int = len;              /* usa sival_int perché stiamo inviando un intero, non 8 byte di stringa */
    sigqueue(pid, SIGUSR1, val_len);      /* invia la lunghezza a sigrx tramite SIGUSR1 */
    ack_received = 0;                     /* azzera il flag prima di mettersi in attesa */
    while(!ack_received)
        sigsuspend(&mask);                /* sospende atomicamente finché non arriva SIGUSR2 */

    /* ── FASE 2: invia i chunk uno per uno ──────────────────────────────── */
    for(int i = 0; i < chunks; i++){
        union sigval value;
        memset(&value, 0, sizeof(value));                               /* azzera tutti i byte: garantisce padding a zero per chunk < 8 byte */
        memcpy(&value.sival_ptr, message + (i * 8), MIN(8, len - i*8)); /* copia il chunk dentro il valore del puntatore (non come indirizzo) */
        sigqueue(pid, SIGUSR1, value);    /* invia il chunk a sigrx */
        ack_received = 0;                 /* azzera il flag prima di attendere il prossimo ACK */
        while(!ack_received)
            sigsuspend(&mask);            /* aspetta la conferma di sigrx prima di inviare il chunk successivo */
    }
    /* tutti i chunk inviati e confermati: la funzione ritorna e il processo termina */
}

int main(int argc, char *argv[]){
    if(argc != 3){                                    /* controlla che siano stati passati esattamente 2 argomenti */
        printf("Usage: %s <pid> <message>\n", argv[0]);
        return -1;
    }

    /* Registra ack_handler come handler per SIGUSR2 (ACK inviato da sigrx).
     * SA_SIGINFO non serve: non abbiamo bisogno di siginfo_t nell'handler dell'ACK. */
    struct sigaction sa;
    sa.sa_handler = ack_handler;   /* handler classico void(int): non ci serve siginfo_t */
    sa.sa_flags = 0;               /* nessun flag speciale */
    sigemptyset(&sa.sa_mask);      /* nessun segnale aggiuntivo bloccato durante l'handler */
    sigaction(SIGUSR2, &sa, NULL); /* installa l'handler per SIGUSR2 */

    pid_t pid = atoi(argv[1]);     /* PID del processo sigrx destinatario */
    char *message = argv[2];       /* stringa da trasmettere, lunghezza arbitraria */
    sigtx(pid, message);           /* avvia il trasferimento */
    return 0;                      /* sigtx termina dopo aver ricevuto l'ultimo ACK */
}
