/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Scriveere un programma pidcmd che stampi i pid dei processi attivi lanciati con una specifica riga di
comando. (Devono coincidere tutti gli argomenti)
es: ll comando "pidcmd less /etc/hostname" deve stampare il numero di processo dei processi attivi
che sono stati lanciati con "less /etc/hostname"
(hint: cercare nelle directory dei processi in /proc i "file" chiamati cmdline)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#define CMDLINE_BUF 65536

/* /proc/<pid>/cmdline contiene gli argomenti separati da '\0' (incluso uno
 * finale): per confrontarli con argv basta ricostruire la stessa sequenza
 * con '\0' come separatore e confrontare con memcmp, lunghezza compresa. */
static int cmdline_matches(pid_t pid, char *target, size_t target_len){
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    FILE *f = fopen(path, "r");
    if(f == NULL){
        return 0;
    }

    static char buf[CMDLINE_BUF];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if(n != target_len){
        return 0;
    }
    return memcmp(buf, target, n) == 0;
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "uso: %s comando [argomenti...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char target[CMDLINE_BUF];
    size_t target_len = 0;
    for(int i = 1; i < argc; i++){
        size_t len = strlen(argv[i]) + 1;
        if(target_len + len > sizeof(target)){
            fprintf(stderr, "pidcmd: riga di comando troppo lunga\n");
            exit(EXIT_FAILURE);
        }
        memcpy(target + target_len, argv[i], len);
        target_len += len;
    }

    DIR *proc = opendir("/proc");
    if(proc == NULL){
        perror("opendir /proc");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while((entry = readdir(proc)) != NULL){
        /* le directory dei processi in /proc hanno nome numerico (il pid) */
        int is_pid = 1;
        for(char *p = entry->d_name; *p != '\0'; p++){
            if(!isdigit((unsigned char)*p)){
                is_pid = 0;
                break;
            }
        }
        if(!is_pid || entry->d_name[0] == '\0'){
            continue;
        }

        pid_t pid = (pid_t)atol(entry->d_name);
        if(cmdline_matches(pid, target, target_len)){
            printf("%d\n", pid);
        }
    }

    closedir(proc);
    exit(EXIT_SUCCESS);
}
