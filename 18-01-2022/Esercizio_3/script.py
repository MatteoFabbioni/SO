#!/usr/bin/env python3
"""
Scrivere uno script bash o un programma python che preso come parametro un pattern (stringa
ASCII) fornisca in output l'elenco dei file che contengono il pattern presenti nel sottoalbero dell
directory corrente.. La lista di output deve essere ordinata dal file più recentemente modificato al file
con ultima modifica più remota.
"""

import os
import sys


def main():
    args = sys.argv[1:]
    if len(args) != 1:
        print("Usage: script.py <pattern>")
        sys.exit(1)

    pattern = args[0].encode("ascii")

    matches = []
    for root, _dirs, files in os.walk("."):
        for name in files:
            path = os.path.join(root, name)
            try:
                with open(path, "rb") as f:
                    content = f.read()
            except OSError as e:
                print(f"Errore su {path}: {e}", file=sys.stderr)
                continue

            if pattern in content:
                matches.append((os.path.getmtime(path), path))

    matches.sort(key=lambda item: item[0], reverse=True)
    for _mtime, path in matches:
        print(path)


if __name__ == "__main__":
    main()
