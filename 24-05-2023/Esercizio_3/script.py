#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash che cerchi all'interno di un sottoalbero se ci sono
link simbolici che indicano lo stesso file. (hint controllare se coincide il numero dell'i-node del file
indicato).
"""

import os
import sys


def main():
    args = sys.argv[1:]
    if len(args) != 1:
        print("Usage: script.py <directory>")
        sys.exit(1)

    root_dir = args[0]
    if not os.path.isdir(root_dir):
        print("Error: <directory> is not a valid directory")
        sys.exit(1)

    targets = {}
    # Mappa (st_dev, st_ino) del file puntato -> lista di symlink che lo referenziano

    for root, dirs, files in os.walk(root_dir):
        for name in dirs + files:
            full_path = os.path.join(root, name)

            if not os.path.islink(full_path):
                continue

            try:
                st = os.stat(full_path)
                # os.stat segue il symlink: serve l'inode del file puntato, non quello del link
            except OSError as e:
                print(f"Error stat-ing target of {full_path}: {e}", file=sys.stderr)
                continue

            key = (st.st_dev, st.st_ino)
            # st_dev insieme a st_ino: lo stesso numero di inode su filesystem diversi
            # identifica file differenti, va sempre confrontata la coppia
            targets.setdefault(key, []).append(full_path)

    for key, links in targets.items():
        if len(links) > 1:
            print(", ".join(links))


if __name__ == "__main__":
    main()
