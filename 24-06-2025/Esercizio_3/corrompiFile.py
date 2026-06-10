#!/usr/bin/env python3
''' TESTO DI TEST '''
"""
Scrivere un programma python o uno script bash che aggiunga
alcune   righe   di   testo   all'inizio   di   tutti   i   file   C,   bash,   python   presenti   nella   directory   corrente
(riconoscibili dal suffisso .c .sh o .py)
Il testo deve comparire come commento coerentemente con la sintassi del linguaggio:
•per i file sorgente C: /* .... */
•per gli script bash: linee con prefisso # (dopo se esiste la prima riga #!)
•per i file sorgente python: ''' .... '''
"""

import os
# Fornisce os.listdir() per elencare i file della directory corrente

import sys
# Fornisce sys.argv per leggere gli argomenti e sys.exit() per uscire con errore

def main():
    args = sys.argv[1:]
    # Prende tutti gli argomenti da riga di comando escludendo il nome dello script

    if len(args) != 1:
        print("Usage: corrompiFile.py <text_to_add>")
        sys.exit(1)
        # Il programma richiede esattamente un argomento: il testo da aggiungere come commento

    text_to_add = args[0]
    # Il testo che verrà inserito come commento all'inizio di ogni file

    for filename in os.listdir('.'):
        # Itera su tutti i file e directory presenti nella directory corrente

        if filename.endswith('.c'):
            # File sorgente C: il commento ha la sintassi /* ... */

            with open(filename, 'r') as f:
                content = f.read()
                # Legge l'intero contenuto del file C

            with open(filename, 'w') as f:
                f.write(f"/* {text_to_add} */\n{content}")
                # Riscrive il file anteponendo il commento C al contenuto originale

        elif filename.endswith('.sh'):
            # Script bash: il commento ha il prefisso #, ma deve stare DOPO l'eventuale shebang

            with open(filename, 'r') as f:
                content = f.read()
                # Legge l'intero contenuto dello script bash

            lines = content.split('\n', 1)
            # Divide il contenuto in due parti: la prima riga e tutto il resto
            # split('\n', 1) esegue al massimo 1 split, quindi lines[0]=prima riga, lines[1]=resto

            with open(filename, 'w') as f:
                if lines[0].startswith('#!'):
                    f.write(f"{lines[0]}\n# {text_to_add}\n{lines[1]}")
                    # Shebang presente (es. #!/bin/bash): lo mantiene in prima riga
                    # e inserisce il commento subito dopo, prima del resto del file
                else:
                    f.write(f"# {text_to_add}\n{content}")
                    # Nessuno shebang: inserisce il commento direttamente all'inizio

        elif filename.endswith('.py'):
            # File Python: il commento usa la sintassi ''' ... ''', ma deve stare DOPO l'eventuale shebang

            with open(filename, 'r') as f:
                content = f.read()
                # Legge l'intero contenuto del file Python

            lines = content.split('\n', 1)
            # Divide il contenuto in prima riga e resto, come per i file bash

            with open(filename, 'w') as f:
                if lines[0].startswith('#!'):
                    f.write(f"{lines[0]}\n''' {text_to_add} '''\n{lines[1]}")
                    # Shebang presente (es. #!/usr/bin/env python3): lo mantiene in prima riga
                    # e inserisce il commento triplo-virgolette subito dopo
                else:
                    f.write(f"''' {text_to_add} '''\n{content}")
                    # Nessuno shebang: inserisce il commento triplo-virgolette all'inizio


if __name__ == "__main__":
    main()
    # Punto di ingresso: esegue main() solo se lo script è lanciato direttamente,
    # non se viene importato come modulo
