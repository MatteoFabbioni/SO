#!/usr/bin/env python3
"""
Scrivere un programma C o uno script bash dremcont.
dremcont  ha due parametri: un file f e una directory d. Tutti i file nella directory d e nelle sue
sottodirectory che hanno contenuto uguale a f devono essere cancellate.
"""

import os
# Fornisce os.path.isfile(), os.path.isdir(), os.walk(), os.path.join(), os.remove()

import sys
# Fornisce sys.argv (lista degli argomenti) e sys.exit() per terminare con errore

def main():
    args = sys.argv[1:]
    # Prende tutti gli argomenti da riga di comando escludendo il nome dello script (sys.argv[0])

    if(len(args) != 2):
        print("Usage: dremcont <file> <directory>")
        sys.exit(1)
        # Il programma richiede esattamente due argomenti: file di riferimento e directory

    file_path_input = args[0]
    # Primo argomento: path del file di riferimento il cui contenuto verrà cercato

    dir_path = args[1]
    # Secondo argomento: path della directory in cui cercare e cancellare i duplicati

    if not os.path.isfile(file_path_input):
        print("Error: <file> is not a valid file")
        sys.exit(1)
        # Verifica che il primo argomento sia un file regolare esistente

    if not os.path.isdir(dir_path):
        print("Error: <directory> is not a valid directory")
        sys.exit(1)
        # Verifica che il secondo argomento sia una directory esistente

    with open(file_path_input, "rb") as f1:
        # Apre il file di riferimento in modalità binaria ("rb"):
        # evita problemi di encoding e confronta i byte esatti
        target_content = f1.read()
        # Legge l'intero contenuto del file di riferimento in memoria
    # Il file viene chiuso automaticamente all'uscita dal blocco with

    for root, dirs, files in os.walk(dir_path):
        # os.walk() scandisce ricorsivamente dir_path:
        # root = directory corrente, dirs = sottodirectory, files = file nella dir corrente

        for name in files:
            # Itera su ogni file nella directory corrente

            file_path = os.path.join(root, name)
            # Costruisce il path completo del file corrente (es. "testdir/subdir/file2.txt")

            with open(file_path, "rb") as f2:
                # Apre il file corrente in modalità binaria per confrontare i byte

                content = f2.read()
                # Legge l'intero contenuto del file corrente

                if content == target_content:
                    # Confronta byte per byte il contenuto del file con quello di riferimento

                    os.remove(file_path)
                    # Contenuto identico: cancella il file


if __name__ == "__main__":
    main()
    # Punto di ingresso: esegue main() solo se lo script è lanciato direttamente,
    # non se viene importato come modulo
