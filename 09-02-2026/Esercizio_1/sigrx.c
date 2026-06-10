/*
 * _POSIX_C_SOURCE 199309L abilita le estensioni POSIX.1b necessarie per
 * sigqueue(), siginfo_t e union sigval, che non fanno parte del C standard.
 */
#define _POSIX_C_SOURCE 199309L
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>

/*
 * Handler invocato alla ricezione di SIGUSR1.
 *
 * Il messaggio non arriva come puntatore a memoria condivisa: sigtx ha
 * copiato i byte della stringa dentro il valore di sival_ptr. Qui li
 * rileggiamo con memcpy nell'array locale 'message', che è già azzerato
 * (inizializzazione = {0}) così il terminatore '\0' è garantito.
 */
void sig_handler(int signum, siginfo_t *info, void *context){
    char message[9] = {0};
    memcpy(message, &info->si_value.sival_ptr, 8); /* estrae i byte dalla parola a 64 bit */
    printf("Received message: %s\n", message);
}

void sigrx(){
    pid_t pid = getpid();
    printf("sigrx running with pid: %d\n", pid);

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;        /* richiede handler esteso (siginfo_t) invece del classico void(*)(int) */
    sa.sa_sigaction = sig_handler;
    sigemptyset(&sa.sa_mask);        /* nessun segnale aggiuntivo bloccato durante l'esecuzione dell'handler */
    sigaction(SIGUSR1, &sa, NULL);

    for(;;){
        pause(); /* sospende il processo fino al prossimo segnale */
    }
    exit(0);
}

int main(){
    sigrx();
    return 0;
}
