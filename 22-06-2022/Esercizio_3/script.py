#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere uno script bash o un programma python che stampi l'elenco dei file di una directory
catalogati per output del comando 'file'. es:

ASCII text: file.txt
directory: mydir
empty: nullfile
PDF document, version 1.6: uno.pdf prova.pdf
"""

import os
import sys
import subprocess


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    directory = sys.argv[1]

    if not os.path.isdir(directory):
        print(f"{directory} non e' una directory", file=sys.stderr)
        sys.exit(1)

    # raggruppa i nomi per descrizione: stesso output di 'file' -> stessa categoria
    categories = {}

    for name in sorted(os.listdir(directory)):
        path = os.path.join(directory, name)
        try:
            # -b (brief) omette il nome del file dall'output, lasciando solo la descrizione
            result = subprocess.run(["file", "-b", path], capture_output=True, text=True, check=True)
        except (OSError, subprocess.CalledProcessError) as e:
            print(f"Errore esecuzione di 'file' su {path}: {e}", file=sys.stderr)
            continue

        description = result.stdout.strip()
        categories.setdefault(description, []).append(name)

    for description, names in categories.items():
        print(f"{description}: {' '.join(names)}")


if __name__ == "__main__":
    main()
