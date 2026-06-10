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
 * Invia 'message' al processo 'pid' usando SIGUSR1 e sigqueue().
 *
 * Il trucco su sistemi a 64 bit: void* è 8 byte, quanto la lunghezza
 * massima del messaggio. I byte della stringa vengono copiati direttamente
 * nel campo sival_ptr di union sigval, usandolo come contenitore di dati
 * (non come puntatore). Il valore intero del campo viene trasmesso dal kernel
 * come parte del payload del segnale, senza condividere memoria tra processi.
 */
void sigtx(pid_t pid, char *message){
    if(strlen(message) > 8){
        printf("Error: message must be at most 8 characters long.\n");
        return;
    }else{
        union sigval value;
        memset(&value, 0, sizeof(value));                        /* azzera i byte del padding */
        memcpy(&value.sival_ptr, message, strlen(message) + 1); /* copia i byte della stringa nel valore del ptr */
        if(sigqueue(pid, SIGUSR1, value) == -1){
            perror("Error sending signal");
            exit(-1);
        }
    }
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: %s <pid> <message>\n", argv[0]);
        return -1;
    }
    pid_t pid = atoi(argv[1]);
    char *message = argv[2];
    sigtx(pid, message);
    return 0;
}
