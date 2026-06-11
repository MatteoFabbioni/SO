#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10  punti
Scrivere un programma python o uno script bash che crei un catalogo dei file presenti nella directory
passata come parametro (o la directory corrente se manca il parametro).
Il catalogo deve essere ordinato in categorie in base alla stringa ritornata dal comando 'file'.
Es:
$ catls
ASCII text:
 testo1
 favourites.txt

directory:
 mydir
 lib

Unicode text, UTF-8 text:
 unitesto
"""

import os
import sys
import subprocess

def main():
    if len(sys.argv) > 2:
        print(f"Uso: {sys.argv[0]} [directory]", file=sys.stderr)
        sys.exit(1)

    target_dir = sys.argv[1] if len(sys.argv) == 2 else "."

    if not os.path.isdir(target_dir):
        print(f"Errore: '{target_dir}' non esiste o non è una directory", file=sys.stderr)
        sys.exit(1)

    catalog = {}

    for name in sorted(os.listdir(target_dir)):
        full_path = os.path.join(target_dir, name)

        # subprocess.run (non os.system) evita di passare per una shell: niente "sh -c"
        result = subprocess.run(["file", "-b", full_path], capture_output=True, text=True)
        category = result.stdout.strip()

        catalog.setdefault(category, []).append(name)

    # dict preserva l'ordine di inserimento: le categorie appaiono nell'ordine
    # in cui sono state incontrate scorrendo i file in ordine alfabetico
    first = True
    for category, names in catalog.items():
        if not first:
            print()
        first = False
        print(f"{category}:")
        for name in names:
            print(f" {name}")

if __name__ == "__main__":
    main()
