#!/usr/bin/env python3
# Shebang: dice al sistema operativo di usare Python 3 per eseguire questo script

"""
Scrivere un programma python o uno script bash che copi un file di testo UTF-8. Nella copia i caratteri
non ASCII devono essere sostituiti con ‘?’.
Es: no8cat filein fileout
se filein contiene:
cioè IY💖
fileout dovrà essere
cio? I?Y
"""
# Docstring: descrizione del problema che lo script risolve

import sys
# Importa il modulo sys, che dà accesso agli argomenti da riga di comando e a sys.exit()

import os
# Importa il modulo os, usato per controllare se un file esiste con os.path.isfile()

def main():
    # Definisce la funzione principale che contiene tutta la logica del programma

    args = sys.argv[1:]
    # sys.argv è la lista degli argomenti passati da terminale (sys.argv[0] è il nome dello script).
    # [1:] prende tutto tranne il primo elemento, cioè solo gli argomenti utente.

    if len(args) != 2:
        # Controlla che l’utente abbia passato esattamente 2 argomenti (filein e fileout)

        print("Usage: no8cat filein fileout")
        # Stampa il messaggio di utilizzo corretto

        sys.exit(1)
        # Termina il programma con codice di errore 1 (convenzione: 0 = successo, != 0 = errore)

    filein, fileout = args
    # Spacchetta i 2 argomenti nelle variabili filein (file sorgente) e fileout (file destinazione)

    if not os.path.isfile(filein):
        # Controlla se filein è un file che esiste realmente sul disco

        print(f"Error: {filein} is not a valid file.")
        # Stampa un messaggio di errore con il nome del file non trovato

        sys.exit(1)
        # Termina il programma con codice di errore

    try:
        # Blocco try: intercetta eventuali errori durante la lettura/scrittura dei file

        with open(filein, encoding='utf-8') as fi, open(fileout, 'w', encoding='ascii') as fo:
            # Apre filein in lettura con codifica UTF-8 (per leggere correttamente accenti, emoji, ecc.)
            # Apre fileout in scrittura con codifica ASCII (scriveremo solo caratteri ASCII, quindi va bene)
            # Il costrutto ‘with’ chiude automaticamente entrambi i file al termine, anche in caso di errore

            for ch in fi.read():
                # fi.read() legge l’intero contenuto del file come stringa Unicode.
                # Il ciclo for itera carattere per carattere su quella stringa.

                fo.write(ch if ord(ch) < 128 else '?')
                # ord(ch) restituisce il valore numerico Unicode del carattere.
                # I caratteri ASCII hanno valore 0-127, quindi ord(ch) < 128 significa "è ASCII".
                # Se il carattere è ASCII lo scrive invariato, altrimenti scrive '?'.

    except Exception as e:
        # Cattura qualsiasi eccezione (es. permessi negati, disco pieno, file corrotto)

        print(f"Error: {e}")
        # Stampa il messaggio di errore dell’eccezione

        sys.exit(1)
        # Termina il programma con codice di errore


if __name__ == "__main__":
    # Questo blocco viene eseguito solo se lo script è lanciato direttamente (non importato come modulo)

    main()
    