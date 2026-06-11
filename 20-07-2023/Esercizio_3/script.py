#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash che data una directory produca un elenco dei file e
delle directory che non potrebbero essere copiati in file system che supportino solo caratteri ascii nei
nomi.
"""

import os
import sys

def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    root = sys.argv[1]
    if not os.path.isdir(root):
        print(f"Errore: '{root}' non e' una directory", file=sys.stderr)
        sys.exit(1)

    for dirpath, dirnames, filenames in os.walk(root):
        for name in dirnames + filenames:
            # un nome e' copiabile su un fs solo-ascii solo se ogni suo
            # carattere e' rappresentabile in ascii (codice < 128)
            if not all(ord(c) < 128 for c in name):
                print(os.path.join(dirpath, name))

if __name__ == "__main__":
    main()
