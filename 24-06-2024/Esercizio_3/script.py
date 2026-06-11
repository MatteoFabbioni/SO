#!/usr/bin/env python3

"""
Esercizio 3: Python o bash: 10 punti
Scrivere un programma python o uno script bash chiamato permdir. permdir prende come
parametro il pathname di una directory e crea nella directory corrente una directory per stringa di
permessi contenente link simbolici ai file con tali permessi.
es: se /tmp/dir è la directory passata come parametro e:
ls -l /tmp/dir
-rw-r--r-- 1 renzo renzo 0 Jun 20 13:23 due
-rw-r----- 1 renzo renzo 0 Jun 20 13:23 quattro
-rwx------ 1 renzo renzo 0 Jun 20 13:23 tre
-rwx------ 1 renzo renzo 0 Jun 20 13:23 uno
il comando permdir deve creare tre directory:
-rw-r--r-- che contiene il link due che punta a /tmp/dir/due
-rw-r----- che contiene il link quattro che punta a /tmp/dir/quattro
-rwx------ che contiene due link uno e tre che puntano agli omonimi file in /tmp/dir
(gestire solo il caso di file, no directory o file speciali)
"""

import sys
import os
import stat


def perm_string(mode):
    # stat.filemode produce anche il carattere di tipo file (es. '-' per
    # i regolari); lo scartiamo per ottenere solo i 9 caratteri rwx richiesti
    return stat.filemode(mode)[1:]


def main():
    if len(sys.argv) != 2:
        print(f"Uso: {sys.argv[0]} <directory>", file=sys.stderr)
        sys.exit(1)

    target = sys.argv[1]

    if not os.path.isdir(target):
        print(f"Errore: {target} non e' una directory", file=sys.stderr)
        sys.exit(1)

    for name in os.listdir(target):
        path = os.path.join(target, name)

        st = os.lstat(path)
        if not stat.S_ISREG(st.st_mode):
            continue

        perms = perm_string(st.st_mode)

        os.makedirs(perms, exist_ok=True)

        link_path = os.path.join(perms, name)
        target_path = os.path.abspath(path)
        if not os.path.lexists(link_path):
            os.symlink(target_path, link_path)


if __name__ == "__main__":
    main()
