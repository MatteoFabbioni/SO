#!/usr/bin/env python3

"""
Scrivere un programma pyhton o uno script bash che calcoli l'ampiezza totale in byte dei file
eseguibili ELF presenti in tutte le directory passate come parametri o nella directory corrente se non
viene specificato alcun parametro.
e.g.
$ elfsize /bin /usr/bin
1682573547
"""

import os
import sys

ELF_MAGIC = b"\x7fELF"


def is_elf(path):
    # Un file e' ELF se i primi 4 byte corrispondono al magic number,
    # quindi basta leggere l'inizio del file e non l'intero contenuto
    try:
        with open(path, "rb") as f:
            return f.read(4) == ELF_MAGIC
    except OSError:
        return False


def total_elf_size(directory):
    total = 0

    for dirpath, _, filenames in os.walk(directory):
        for name in filenames:
            full = os.path.join(dirpath, name)
            # os.walk segue solo le directory reali: i file vengono filtrati qui
            # con isfile per scartare symlink rotti o altri tipi speciali
            if os.path.isfile(full) and is_elf(full):
                total += os.path.getsize(full)

    return total


def main():
    directories = sys.argv[1:] if len(sys.argv) > 1 else ["."]

    grand_total = 0

    for directory in directories:
        if not os.path.isdir(directory):
            print(f"Errore: '{directory}' non e' una directory.", file=sys.stderr)
            sys.exit(1)
        grand_total += total_elf_size(directory)

    print(grand_total)


if __name__ == "__main__":
    main()
