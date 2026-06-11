/**
Esercizio 2: Linguaggio C: 10 punti
Scrivere un programma che dato il numero di un processo attivo ne lanci uno uguale (lo stesso
eseguibile con gli stessi parametri, lo stesso environment e nella stessa directory corrente.
(hint: cercare nella directory del processo da clonare in /proc)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUF_SIZE 65536
#define MAX_ITEMS 1024

/* /proc/<pid>/cmdline e /proc/<pid>/environ hanno lo stesso formato:
 * una sequenza di stringhe terminate da '\0'. Questa funzione legge il
 * file e costruisce un array di puntatori (stile argv/envp) dentro buf. */
static int read_proc_list(const char *path, char *buf, size_t buf_size, char *items[], int max_items){
    FILE *f = fopen(path, "r");
    if(f == NULL){
        perror(path);
        return -1;
    }

    size_t n = fread(buf, 1, buf_size - 1, f);
    fclose(f);
    buf[n] = '\0';

    int count = 0;
    size_t pos = 0;
    while(pos < n && count < max_items - 1){
        items[count++] = buf + pos;
        pos += strlen(buf + pos) + 1;
    }
    items[count] = NULL;
    return count;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "uso: %s pid\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *endptr;
    long pid = strtol(argv[1], &endptr, 10);
    if(*endptr != '\0' || pid <= 0){
        fprintf(stderr, "%s: pid non valido: %s\n", argv[0], argv[1]);
        exit(EXIT_FAILURE);
    }

    char proc_dir[64];
    snprintf(proc_dir, sizeof(proc_dir), "/proc/%ld", pid);
    struct stat st;
    if(stat(proc_dir, &st) == -1){
        fprintf(stderr, "%s: il processo %ld non esiste\n", argv[0], pid);
        exit(EXIT_FAILURE);
    }

    char exe_path[64], cwd_path[64], cmdline_path[64], environ_path[64];
    snprintf(exe_path, sizeof(exe_path), "/proc/%ld/exe", pid);
    snprintf(cwd_path, sizeof(cwd_path), "/proc/%ld/cwd", pid);
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%ld/cmdline", pid);
    snprintf(environ_path, sizeof(environ_path), "/proc/%ld/environ", pid);

    char exe_target[PATH_MAX];
    ssize_t len = readlink(exe_path, exe_target, sizeof(exe_target) - 1);
    if(len == -1){
        perror("readlink exe");
        exit(EXIT_FAILURE);
    }
    exe_target[len] = '\0';

    static char cmdline_buf[BUF_SIZE];
    char *cmd_argv[MAX_ITEMS];
    if(read_proc_list(cmdline_path, cmdline_buf, sizeof(cmdline_buf), cmd_argv, MAX_ITEMS) == -1){
        exit(EXIT_FAILURE);
    }

    static char environ_buf[BUF_SIZE];
    char *cmd_envp[MAX_ITEMS];
    if(read_proc_list(environ_path, environ_buf, sizeof(environ_buf), cmd_envp, MAX_ITEMS) == -1){
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();
    if(child == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(child == 0){
        if(chdir(cwd_path) == -1){
            perror("chdir");
            exit(EXIT_FAILURE);
        }
        execve(exe_target, cmd_argv, cmd_envp);
        perror("execve");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
