#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: (10 punti)
Scrivere uno script in grado si cercare all'interno di un sottoalbero del file system il file modificato più
di recente e quello la cui ultima modifca è avvenuta più anticamente.
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

    most_recent_path = None
    most_recent_mtime = None
    oldest_path = None
    oldest_mtime = None

    for root, dirs, files in os.walk(root_dir):
        for name in files:
            # solo file regolari: l'enunciato chiede "il file", le directory
            # vanno escluse dal confronto (il loro mtime cambia ad ogni
            # creazione/rimozione di una entry al loro interno)
            full_path = os.path.join(root, name)

            try:
                st = os.lstat(full_path)
                # lstat: non si segue il symlink, si valuta la entry stessa
            except OSError as e:
                print(f"Error stat-ing {full_path}: {e}", file=sys.stderr)
                continue

            mtime = st.st_mtime
            if most_recent_mtime is None or mtime > most_recent_mtime:
                most_recent_mtime = mtime
                most_recent_path = full_path
            if oldest_mtime is None or mtime < oldest_mtime:
                oldest_mtime = mtime
                oldest_path = full_path

    if most_recent_path is None:
        print("No files found in subtree")
        sys.exit(0)

    print(f"Most recently modified: {most_recent_path}")
    print(f"Least recently modified: {oldest_path}")


if __name__ == "__main__":
    main()
