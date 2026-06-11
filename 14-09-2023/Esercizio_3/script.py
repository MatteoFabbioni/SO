#!/usr/bin/env python3
"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash che data una directory passata come parametro
produca una lista dei link simbolici presenti nel sottoalbero che fanno riferimento allo stesso file.
Esempio, in questo caso:
$ ls -lR /tmp/test
/tmp/test:
total 4
drwxr-xr-x 2 renzo renzo 4096 Sep 10 15:45 d
-rw-r--r-- 1 renzo renzo 0 Sep 10 15:41 file1
-rw-r--r-- 1 renzo renzo 0 Sep 10 15:41 file2
lrwxrwxrwx 1 renzo renzo 5 Sep 10 15:42 sl1 -> file1
lrwxrwxrwx 1 renzo renzo 5 Sep 10 15:42 sl1bis -> file1
lrwxrwxrwx 1 renzo renzo 5 Sep 10 15:49 sl2 -> file2

/tmp/test/d:
total 0
lrwxrwxrwx 1 renzo renzo 15 Sep 10 15:45 gsld -> /tmp/test/file1
lrwxrwxrwx 1 renzo renzo 8 Sep 10 15:43 sld -> ../file1

il programma lanciato con parametro /tmp/test deve trovare che sl1, sl1bis, d/sld e d/gllsd indicano lo
stesso file. (similmente dovrebbe rilevare altri insiemi di link simbolici che indicano lo stesso file)
"""

import os
import sys
import stat


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
