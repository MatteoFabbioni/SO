#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: (10 punti)
Scrivere un programma/script che faccia la lista riscorsiva dei file in un sottoalbero riportando in
ordine alfabetico per nome di file in quale/quali sottodirectory compare.

e.g.
rlsr mydir
ciao: . ./a
mare: ./a ./b
sole: .

Significato: un file con nome ciao esite nella directory mydir ma anche all'interno della sottodirectory
a (esistono cioe' i file mydir/ciao e mydir/a/ciao).
"""

import os
import sys


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    root = sys.argv[1]

    if not os.path.isdir(root):
        print(f"{sys.argv[0]}: '{root}' non e' una directory valida", file=sys.stderr)
        sys.exit(1)

    # mappa nome file -> insieme di directory (relative a root, stile "." "./a") in cui compare
    occorrenze = {}

    for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
        rel = os.path.relpath(dirpath, root)
        rel = "." if rel == "." else "./" + rel
        for name in filenames:
            occorrenze.setdefault(name, set()).add(rel)

    for name in sorted(occorrenze):
        dirs = " ".join(sorted(occorrenze[name]))
        print(f"{name}: {dirs}")


if __name__ == "__main__":
    main()
