/**
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Sia dato questo programma hw.c (vi viene fornito in /public/hw.c)
		#include <stdio.h>
		int main(int argc, char*argv[]) {
			printf("hello world:");
			for(argv++, argv--; argc > 0; argv++, argc--)
				printf(" %s",*argv);
			printf("\n");
			return 0;
		}
Il programma hw.c può essere compilato come libreria dinamica:
		gcc --shared -o hw.so hw.c
La libreria dinamica non è un eseguibile
		$ ./hw.so 1 2 3 4
		Segmentation fault
ma può essere caricata a tempo di esecuzione tramite dlopen. Scrivere un programma "lancia" in
grado di eseguire il codice di hw.so
		$ ./lancia hw.so 1 2 3 4
		hello world: hw.so 1 2 3 4
(suggerimenti: dlopen non cerca nella directory corrente, occorre passare il path assoluto della libreria.
"main" in hw.so è una normale funzione: occorre cercare l'indirizzo della funzione main nella libreria
ed invocarla,)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <limits.h>

typedef int (*main_func_t)(int, char **);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <libreria.so> [args...]\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (lstat(argv[1], &st) == -1) {
        perror("lstat");
        return 1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a regular file\n", argv[1]);
        return 1;
    }

    char abspath[PATH_MAX];
    if (realpath(argv[1], abspath) == NULL) {
        /* dlopen non cerca nella directory corrente: serve il path assoluto */
        perror("realpath");
        return 1;
    }

    void *handle = dlopen(abspath, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }

    dlerror();
    main_func_t hw_main = (main_func_t) dlsym(handle, "main");
    char *err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "dlsym: %s\n", err);
        dlclose(handle);
        return 1;
    }

    int hw_argc = argc - 1;
    char **hw_argv = argv + 1;
    int ret = hw_main(hw_argc, hw_argv);

    dlclose(handle);
    return ret;
}
