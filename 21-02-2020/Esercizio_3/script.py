#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: (10 punti)
Scrivere un programma python o uno script bash che produca il listato del contenuto di una directory
in ordine di suffisso.
"""

import sys
import os


def suffix(name):
    # os.path.splitext include il punto nel suffisso (es. ".txt");
    # i file senza estensione hanno suffisso vuoto e vanno ordinati per primi
    return os.path.splitext(name)[1]


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    dir_path = sys.argv[1]

    if not os.path.exists(dir_path):
        print(f"Errore: {dir_path} non esiste", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(dir_path):
        print(f"Errore: {dir_path} non e' una directory", file=sys.stderr)
        sys.exit(1)

    try:
        entries = os.listdir(dir_path)
    except OSError as e:
        print(f"Errore: {e}", file=sys.stderr)
        sys.exit(1)

    entries.sort(key=suffix)

    for name in entries:
        print(name)


if __name__ == "__main__":
    main()
