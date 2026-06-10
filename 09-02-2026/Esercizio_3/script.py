#!/usr/bin/env python3
"""
Scrivere uno script bash o un programma python che preso come parametro un pattern (stringa
ASCII) fornisca in output l'elenco dei file del sottoalbero che ha come radice la directory corrente che
nel loro contenuto includano il pattern.. La lista di output deve essere ordinata dal file con tempo di
ultima modifica più antico al file con ultima modifica più recente.
"""

import sys               # sys.argv per leggere gli argomenti da riga di comando, sys.exit per uscire con codice di errore
import os                # os.walk per visitare l'albero di directory, os.path.join per costruire path, os.stat per i metadati

def main():
    args = sys.argv[1:]      # sys.argv[0] è il nome dello script, da [1:] in poi sono i parametri passati dall'utente
    if len(args) != 1:       # controlla che sia stato passato esattamente un argomento (il pattern)
        sys.exit(1)          # esce con codice 1 (errore) se il numero di argomenti è sbagliato

    pattern = args[0].encode()   # converte il pattern da stringa a bytes: necessario per cercarlo in file aperti in modalità binaria ("rb")

    files = []   # lista che accumula i path dei file che contengono il pattern
    for dirpath, dirnames, filenames in os.walk("."):   # visita ricorsivamente il sottoalbero dalla directory corrente ("."); dirpath=cartella corrente, dirnames=sottocartelle, filenames=file
        for name in filenames:                          # itera solo sui file (non sulle directory)
            full = os.path.join(dirpath, name)          # costruisce il path completo del file unendo la directory e il nome
            try:
                with open(full, "rb") as f:             # apre in modalità binaria ("rb"): evita crash su file non testuali (ELF, immagini, ecc.)
                    if pattern in f.read():             # legge tutto il contenuto del file e verifica se il pattern (bytes) è presente
                        files.append(full)              # aggiunge il path alla lista solo se il pattern è trovato
            except (PermissionError, FileNotFoundError):   # alcuni file (es. in /proc) spariscono o sono inaccessibili durante la lettura
                pass                                        # ignora silenziosamente i file che non si riescono ad aprire

    files.sort(key=lambda p: os.stat(p).st_mtime)   # ordina la lista per tempo di ultima modifica (st_mtime); reverse=False (default) = dal più vecchio al più recente

    for f in files:   # itera sui file trovati, già ordinati
        print(f)      # stampa il path di ogni file su stdout, uno per riga

if __name__ == "__main__":   # punto di ingresso: esegue main() solo se lo script è lanciato direttamente (non importato come modulo)
    main()
