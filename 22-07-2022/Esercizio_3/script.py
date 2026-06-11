#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Il programma python o lo script bash deve eseguire uno dopo l'altro tutti gli script presenti nella
directory passata come parametro (o la current directory se manca il parametro) ma non gli eseguibili
binari di tipo ELF.
"""

import os
import sys
import subprocess


def is_elf(path):
    try:
        with open(path, "rb") as f:
            magic = f.read(4)
    except OSError as e:
        print(f"Errore apertura {path}: {e}", file=sys.stderr)
        return False
    return magic == b"\x7fELF"


def main():
    directory = sys.argv[1] if len(sys.argv) > 1 else "."

    if not os.path.isdir(directory):
        print(f"{directory} non e' una directory", file=sys.stderr)
        sys.exit(1)

    for name in sorted(os.listdir(directory)):
        path = os.path.join(directory, name)

        if not os.path.isfile(path):
            continue
        if not os.access(path, os.X_OK):
            continue
        if is_elf(path):
            continue

        try:
            subprocess.run([path])
        except OSError as e:
            print(f"Errore esecuzione di {path}: {e}", file=sys.stderr)


if __name__ == "__main__":
    main()
