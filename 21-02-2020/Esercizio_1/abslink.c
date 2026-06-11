/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti.
Il comando abslink, da implementare, deve sostituire un link simbolico con uno equivalente che sia
un riferimento ad un path assoluto.
Ad esempio: se mylink e' nella directory /home/user e punta a myfile, 'abspath mylink" deve sostituire
mylink con un symbolic link a /home/user/myfile.
Hint: considerate l'uso della funzione realpath.
 */
#define _DEFAULT_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<limits.h>

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <symlink>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *link_path = argv[1];

    struct stat link_stat;
    if(lstat(link_path, &link_stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }

    if(!S_ISLNK(link_stat.st_mode)){
        fprintf(stderr, "Error: %s is not a symbolic link\n", link_path);
        exit(EXIT_FAILURE);
    }

    /* realpath risolve il target (anche se relativo o con symlink a catena)
     * nel suo path assoluto canonico */
    char abs_target[PATH_MAX];
    if(realpath(link_path, abs_target) == NULL){
        perror("realpath");
        exit(EXIT_FAILURE);
    }

    if(unlink(link_path) == -1){
        perror("unlink");
        exit(EXIT_FAILURE);
    }

    if(symlink(abs_target, link_path) == -1){
        perror("symlink");
        exit(EXIT_FAILURE);
    }

    return 0;
}
