#!/usr/bin/env python3

"""
Scrivere un programma Python o uno script bash che, dati il pathname di un file f e di una directory d
stampi  a seconda del proprio nome:
• l'elenco dei link simbolici che puntano a f presenti nella directory d se chiamato come cksimlink
    cksymlink /tmp/file /tmp/dir
• l'elenco dei link fisici di f presenti nella directory d se chiamato come cklink
    cklink /tmp/file /tmp/dir
"""

import os
import sys

def main():
    if(len(sys.argv) != 3):                          # deve ricevere esattamente 2 argomenti: file e directory
        print("Uso: ./cksymlink <file> <directory> o ./cklink <file> <directory>")
        return

    if(sys.argv[0] == "./cksymlink"):                # lo script è stato invocato come cksymlink
        file_path = sys.argv[1]                      # pathname del file target
        dir_path = sys.argv[2]                       # pathname della directory da scorrere
        if not os.path.isfile(file_path):            # verifica che file_path esista e sia un file regolare
            print(f"{file_path} non è un file valido.")
            return
        if not os.path.isdir(dir_path):              # verifica che dir_path esista e sia una directory
            print(f"{dir_path} non è una directory valida.")
            return
        for entry in os.listdir(dir_path):           # itera su tutti gli elementi della directory
            full_path = os.path.join(dir_path, entry)                    # costruisce il path completo dell'entry
            if os.path.islink(full_path) and os.path.realpath(full_path) == os.path.realpath(file_path):
                # os.path.islink: vero se è un link simbolico
                # os.path.realpath: risolve il path reale (gestisce anche symlink relativi)
                # se il link punta allo stesso file reale di file_path, lo stampa
                print(full_path)

    elif(sys.argv[0] == "./cklink"):                 # lo script è stato invocato come cklink
        file_path = sys.argv[1]                      # pathname del file target
        dir_path = sys.argv[2]                       # pathname della directory da scorrere
        if not os.path.isfile(file_path):            # verifica che file_path esista e sia un file regolare
            print(f"{file_path} non è un file valido.")
            return
        if not os.path.isdir(dir_path):              # verifica che dir_path esista e sia una directory
            print(f"{dir_path} non è una directory valida.")
            return
        for entry in os.listdir(dir_path):           # itera su tutti gli elementi della directory
            full_path = os.path.join(dir_path, entry)                    # costruisce il path completo dell'entry
            if os.path.isfile(full_path) and os.stat(full_path).st_ino == os.stat(file_path).st_ino:
                # os.stat().st_ino: numero di inode del file
                # due file con lo stesso inode sono hard link dello stesso dato su disco
                print(full_path)


if __name__ == "__main__":
    main()                                           # punto di ingresso dello script
