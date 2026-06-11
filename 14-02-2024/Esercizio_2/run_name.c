/**
Esercizio 2: Linguaggio C: 10 punti
Il programma run_name deve cercare nel sottoalbero della directory corrente tutti i file eseguibili con
un nome file specifico (primo parametro di run_name) e li deve mettere in esecuzione uno dopo l'altro
passando i successivi parametri.
Ad esempio il comando:
./run_name testprog a b c
deve cercare i file eseguibili chiamati testprog nell'albero della directory corrente. Poniamo
siano ./testprog, ./dir1/testprog, ./dir/dir3/testprog, run_name deve eseguire
testprog a b c
per 3 volte. Nella prima esecuzione la working directory deve essere la dir corrente '.', la seconda
deve avere come working directory './dir1' e la terza './dir2/dir3'.
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX/BSD necessarie per opendir/readdir su Linux */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

/* Esegue target nella directory dirpath, passando argv[2..argc-1] come argomenti */
void run_in_dir(const char *dirpath, const char *full_path, int argc, char *argv[]){
    pid_t pid = fork();

    if(pid == -1){
        perror("fork");
        return;
    }

    if(pid == 0){
        if(chdir(dirpath) == -1){
            perror("chdir");
            exit(EXIT_FAILURE);
        }

        /* argv[1] è il nome del file da eseguire (es. testprog), argv[2..] i suoi parametri */
        char *exec_args[argc];
        exec_args[0] = argv[1];
        int i;
        for(i = 2; i < argc; i++){
            exec_args[i - 1] = argv[i];
        }
        exec_args[argc - 1] = NULL;

        /* execv richiede il path del binario relativo alla nuova cwd: basta il nome del file */
        execv(argv[1], exec_args);

        perror("execv");
        exit(EXIT_FAILURE);
    }

    /* full_path non serve al figlio: viene usato solo a scopo di debug/eventuali estensioni */
    (void)full_path;

    int status;
    if(waitpid(pid, &status, 0) == -1){
        perror("waitpid");
    }
}

void search(const char *dirpath, int argc, char *argv[]){
    DIR *dir = opendir(dirpath);
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        /* lstat: non segue i symlink, evita di seguire link che puntano fuori
           dall'albero o che creano cicli nel filesystem */
        if(lstat(full_path, &st) == -1){
            perror("lstat");
            continue;
        }

        if(S_ISDIR(st.st_mode)){
            search(full_path, argc, argv);
        } else if(S_ISREG(st.st_mode) && strcmp(entry->d_name, argv[1]) == 0){
            if(access(full_path, X_OK) == 0){
                run_in_dir(dirpath, full_path, argc, argv);
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "Usage: %s <nome_file> [parametri...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    search(".", argc, argv);

    return 0;
}
