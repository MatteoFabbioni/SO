/**
 Scrivere il programma C modifcmp che confronta il tempo dell'ultima modifica fra file
modifcmp ammette molteplici modalità di funzionamento,
•se viene chiamato con il pathname di un file come solo argomento elenca i pathname di tutti i
file nel sottoalbero della directory corrente aggiornati più recentemente rispetto al file passato
come parametro (es:  modifcmp myfile)
•se viene chiamato   con i pathname di due file come   parametri stampa il pathname del
secondo file se è stato aggiornato più recentemente del primo (es:   modifcmp myfile1
tmp/myfile2)
•se viene chiamato con un pathname di un file e uno di una directory elenca il pathname di
tutti   i   file   nel   sottoalbero   che   ha   come   radice   il   secondo   pathname   aggiornati   più
recentemente rispetto al file passato come primo parametro (es:  modifcmp myfile mydir)
 */

#define _DEFAULT_SOURCE
/* Abilita le estensioni POSIX e GNU: necessario per rendere visibile PATH_MAX da <limits.h> */

#include<stdio.h>
/* Fornisce printf() e perror() per stampare output e messaggi di errore */

#include<stdlib.h>
/* Libreria standard C: non usata direttamente ma buona pratica includerla */

#include<sys/stat.h>
/* Fornisce stat(), struct stat e le macro S_ISREG(), S_ISDIR() per ispezionare i file */

#include<dirent.h>
/* Fornisce opendir(), readdir(), closedir() e struct dirent per scorrere le directory */

#include<string.h>
/* Fornisce strcmp() per confrontare stringhe (usato per saltare "." e "..") */

#include<unistd.h>
/* Fornisce costanti e tipi POSIX standard */

#include<time.h>
/* Fornisce il tipo time_t usato per rappresentare i timestamp di modifica */

#include<limits.h>
/* Fornisce PATH_MAX: la lunghezza massima di un pathname sul sistema */

void walk(const char *path, time_t ref_time){
/* Funzione ricorsiva che scorre il sottoalbero di 'path' e stampa i file
   modificati più recentemente di ref_time */

    DIR *dir = opendir(path);
    /* Apre la directory 'path' e restituisce un puntatore DIR per iterarla */

    if(!dir){
        /* Se opendir fallisce (es. permessi negati) dir è NULL */

        perror("Error opening directory");
        /* Stampa il messaggio di errore di sistema corrispondente all'errno corrente */

        return;
        /* Esce dalla funzione senza crashare: gli altri rami dell'albero continuano */
    }

    struct dirent *entry;
    /* Struttura che conterrà le informazioni di ogni voce letta dalla directory */

    while((entry = readdir(dir)) != NULL){
        /* readdir() restituisce la prossima voce della directory, NULL quando finisce */

        if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")){
            /* Salta le voci speciali "." (directory corrente) e ".." (directory padre)
               per evitare loop infiniti nella ricorsione */

            continue;
            /* Passa alla prossima iterazione del while */
        }

        char full_path[PATH_MAX];
        /* Buffer che conterrà il pathname completo della voce corrente */

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        /* Costruisce il pathname completo concatenando la directory corrente e il nome della voce.
           snprintf limita la scrittura a sizeof(full_path) byte per evitare buffer overflow */

        struct stat st;
        /* Struttura che conterrà le informazioni (tipo, mtime, ecc.) del file corrente */

        if(stat(full_path, &st) == -1) continue;
        /* stat() riempie 'st' con i metadati di full_path.
           Se fallisce (es. symlink rotto) restituisce -1: si salta la voce e si continua */

        if(S_ISDIR(st.st_mode)){
            /* S_ISDIR controlla se il file è una directory, usando il campo st_mode */

            walk(full_path, ref_time);
            /* Chiamata ricorsiva: scende nel sottoalbero della sottodirectory trovata */

        }else{
            /* La voce è un file regolare (o altro tipo non-directory) */

            if(st.st_mtime > ref_time){
                /* st.st_mtime è il timestamp dell'ultima modifica del file (in secondi Unix).
                   Se è maggiore di ref_time, il file è più recente del file di riferimento */

                printf("%s\n", full_path);
                /* Stampa il pathname completo del file più recente */
            }
        }
    }

    closedir(dir);
    /* Chiude il descrittore della directory liberando le risorse di sistema */
}

int main(int argc, char *argv[]){
/* Punto di ingresso del programma.
   argc = numero di argomenti (incluso il nome del programma)
   argv = array di stringhe con gli argomenti */

    if(argc == 1){
        /* Nessun argomento passato: solo il nome del programma */

        printf("Usage: %s <file1> [<file2>|<directory>]\n", argv[0]);
        /* Stampa le istruzioni d'uso. argv[0] è il nome dell'eseguibile */

        return -1;
        /* Termina con codice di errore */

    }else if(argc == 2){
        /* Un solo argomento: modalità "scansiona la directory corrente" */

        struct stat st;
        /* Struttura per i metadati del file passato come argomento */

        if(stat(argv[1], &st) == -1){
            /* Ottiene i metadati di argv[1]; se fallisce il file non esiste o non è accessibile */

            perror("Error stat file");
            /* Stampa l'errore di sistema */

            return -1;
            /* Termina con codice di errore */
        }

        if(!S_ISREG(st.st_mode)){
            /* Controlla che argv[1] sia un file regolare (non una directory, symlink, ecc.) */

            printf("Error: %s is not a regular file.\n", argv[1]);
            /* Segnala che il primo argomento deve essere un file regolare */

            return -1;
            /* Termina con codice di errore */
        }

        walk(".", st.st_mtime);
        /* Scansiona ricorsivamente la directory corrente (".") cercando file
           più recenti di st.st_mtime (il timestamp di modifica di argv[1]) */

    }else if(argc == 3){
        /* Due argomenti: modalità "file vs file" oppure "file vs directory" */

        struct stat st1, st2;
        /* Due strutture per i metadati dei due argomenti */

        if(stat(argv[1], &st1) == -1){
            /* Ottiene i metadati del primo argomento */

            perror("Error stat file1");
            return -1;
        }

        if(!S_ISREG(st1.st_mode)){
            /* Il primo argomento deve sempre essere un file regolare */

            printf("Error: %s is not a regular file.\n", argv[1]);
            return -1;
        }

        if(stat(argv[2], &st2) == -1){
            /* Ottiene i metadati del secondo argomento (file o directory) */

            perror("Error stat file2/directory");
            return -1;
        }

        if(S_ISREG(st2.st_mode)){
            /* Il secondo argomento è un file regolare: confronto diretto tra i due file */

            if(st2.st_mtime > st1.st_mtime){
                /* Se il secondo file è più recente del primo, lo stampa */

                printf("%s\n", argv[2]);
            }

        }else if(S_ISDIR(st2.st_mode)){
            /* Il secondo argomento è una directory: scansiona il suo sottoalbero */

            walk(argv[2], st1.st_mtime);
            /* Cerca nel sottoalbero di argv[2] tutti i file più recenti di argv[1] */

        }else{
            /* Il secondo argomento non è né un file regolare né una directory */

            printf("Error: %s is neither a regular file nor a directory.\n", argv[2]);
            return -1;
        }

    }else{
        /* Più di 3 argomenti: uso non valido */

        printf("Usage: %s <file1> [<file2>|<directory>]\n", argv[0]);
        return -1;
    }

    return 0;
    /* Termina il programma con successo (codice 0) */
}
