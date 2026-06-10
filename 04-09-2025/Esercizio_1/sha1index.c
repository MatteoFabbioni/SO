/**
Scrivere un programma sha1index che ha al più un
parametro. (se non ha parametro opera sulla directory corrente, altrimenti sulla directory indicata dal
parametro)
Per ogni file f ordinario nella directory il programma crea un link simbolico in una sottodirectory
nascosta di nome ".sha1index" che punta al file e ha come nome la hash sha1 del contenuto.
Es. data nella dir corrente due file f1 e f2, il programma  sha1index crea nella directory .sha1index
due link simbolici:
.sha1index/5e180efdd44e3a3585834b6bd618ef7c5a462d9a che punta a f1 e
.sha1index/82442bcd9a1e36899d43c04f79491cd616f7b30a che punta a f2
(i valori delle hash sono solo a titolo di esempio, rappresentano le hash  del contenuto di f1 e di f2)
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

int main(int argc, char *argv[]) {

    char *dir_path = (argc > 1) ? argv[1] : ".";
    /* Se viene passato un argomento usa quello come directory di lavoro, altrimenti usa "." */

    DIR *dir = opendir(dir_path);
    /* Apre la directory dir_path per poterne leggere le voci con readdir() */

    if(dir == NULL) {
        /* Se opendir fallisce (directory inesistente o permessi negati) dir è NULL */

        perror("opendir");
        /* Stampa il messaggio di errore di sistema corrispondente all'errno corrente */

        return EXIT_FAILURE;
        /* Termina il programma con codice di errore */
    }

    char sha1_dir[PATH_MAX];
    /* Buffer per contenere il path completo della directory .sha1index */

    snprintf(sha1_dir, sizeof(sha1_dir), "%s/.sha1index", dir_path);
    /* Costruisce il path ".sha1index" dentro la directory di lavoro.
       snprintf limita la scrittura a PATH_MAX byte per evitare buffer overflow */

    if(mkdir(sha1_dir, 0755) == -1 && errno != EEXIST) {
        /* mkdir() crea la directory .sha1index con permessi rwxr-xr-x (0755).
           Se restituisce -1 ma errno è EEXIST, la directory c'era già: va bene.
           Qualsiasi altro errore (es. permessi negati) è fatale */

        perror("mkdir .sha1index");
        /* Stampa il messaggio di errore di sistema */

        closedir(dir);
        /* Chiude la directory prima di uscire per liberare le risorse */

        return EXIT_FAILURE;
        /* Termina il programma con codice di errore */
    }

    struct dirent *entry;
    /* Puntatore alla struttura che conterrà le informazioni di ogni voce letta */

    while((entry = readdir(dir)) != NULL) {
        /* readdir() restituisce la prossima voce della directory ad ogni chiamata,
           NULL quando non ci sono più voci */

        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        /* Salta le voci speciali "." (directory corrente) e ".." (directory padre)
           per evitare di processare ricorsivamente le directory di sistema */

        if(strcmp(entry->d_name, ".sha1index") == 0)
            continue;
        /* Salta la directory .sha1index stessa per non processarne il contenuto */

        char file_path[PATH_MAX];
        /* Buffer per il path completo della voce corrente */

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
        /* Costruisce il path completo concatenando la directory di lavoro e il nome della voce */

        struct stat st;
        /* Struttura che conterrà i metadati (tipo, dimensione, ecc.) della voce corrente */

        if(stat(file_path, &st) == -1) {
            /* stat() riempie 'st' con i metadati del file; -1 indica un errore */

            perror("stat");
            /* Stampa l'errore e passa alla voce successiva */

            continue;
            /* Salta questa voce e continua con la prossima */
        }

        if(!S_ISREG(st.st_mode))
            continue;
        /* S_ISREG controlla se la voce è un file regolare (non directory, symlink, device...).
           Se non lo è, la salta: il programma opera solo su file ordinari */

        int pipefd[2];
        /* Crea una pipe: pipefd[0] è l'estremità di lettura, pipefd[1] quella di scrittura.
           Serve come canale di comunicazione tra padre e figlio */

        if (pipe(pipefd) == -1) { perror("pipe"); continue; }
        /* Se pipe() fallisce salta questo file e passa al prossimo */

        pid_t pid = fork();
        /* fork() duplica il processo corrente.
           Restituisce 0 al figlio, il PID del figlio al padre, -1 in caso di errore */

        if (pid == -1) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            /* In caso di errore chiude entrambe le estremità della pipe prima di continuare */
            continue;
        }

        if (pid == 0) {
            /* Blocco eseguito solo dal processo figlio */

            close(pipefd[0]);
            /* Il figlio non legge dalla pipe: chiude l'estremità di lettura */

            dup2(pipefd[1], STDOUT_FILENO);
            /* Redirige lo stdout del figlio verso la pipe:
               da questo momento qualsiasi cosa sha1sum stampa su stdout finisce nella pipe */

            close(pipefd[1]);
            /* Il descrittore originale della pipe non serve più: è già duplicato su STDOUT_FILENO */

            char *args[] = {"sha1sum", file_path, NULL};
            /* Array di argomenti per execvp: argv[0] è il nome del programma,
               argv[1] è il file da hashare, NULL termina la lista */

            execvp("sha1sum", args);
            /* Sostituisce il processo figlio con sha1sum.
               Se ha successo questa riga non ritorna mai */

            perror("execvp");
            exit(EXIT_FAILURE);
            /* Queste due righe vengono raggiunte solo se execvp fallisce (es. sha1sum non trovato) */
        }

        /* Da qui in poi: solo il processo padre */

        close(pipefd[1]);
        /* Il padre non scrive nella pipe: chiude l'estremità di scrittura.
           Necessario affinché la read() del padre riceva EOF quando sha1sum termina */

        char output[256];
        ssize_t n = read(pipefd[0], output, sizeof(output) - 1);
        /* Legge l'output di sha1sum dalla pipe.
           L'output ha il formato: "a3f1...40char  nomefile\n" */

        close(pipefd[0]);
        /* Chiude l'estremità di lettura della pipe dopo aver letto */

        wait(NULL);
        /* Attende la terminazione del processo figlio (sha1sum) per evitare processi zombie.
           NULL indica che non ci interessa il codice di uscita */

        if (n < 40) continue;
        /* Se l'output è più corto di 40 caratteri c'è stato un errore: salta il file */

        output[40] = '\0';
        /* sha1sum stampa "hash  nomefile": tronca dopo i primi 40 caratteri (l'hash hex)
           ignorando il resto della riga */

        char link_path[PATH_MAX + 64];
        snprintf(link_path, sizeof(link_path), "%s/%.40s", sha1_dir, output);
        /* Costruisce il path del symlink: ".sha1index/<hash40caratteri>" */
        /* Costruisce il path del symlink: ".sha1index/<hash40caratteri>" */

        char link_target[PATH_MAX];
        /* Buffer per il target (destinazione) del symlink */

        snprintf(link_target, sizeof(link_target), "../%s", entry->d_name);
        /* Il target è un path relativo: "../nomefile".
           Partendo da .sha1index/, "../" risale alla directory padre dove si trova il file originale.
           Usare un path relativo fa funzionare il symlink anche se si sposta l'intera directory */

        if(symlink(link_target, link_path) == -1 && errno != EEXIST)
            perror("symlink");
        /* symlink() crea il link simbolico link_path → link_target.
           Se restituisce -1 con EEXIST, un symlink con lo stesso hash esiste già (due file con
           contenuto identico): comportamento corretto, si ignora silenziosamente.
           Qualsiasi altro errore viene stampato con perror() */
    }

    closedir(dir);
    /* Chiude il descrittore della directory liberando le risorse di sistema */

    return EXIT_SUCCESS;
    /* Termina il programma con codice di successo (0) */
}
