#!/bin/bash

# Definir la línea de copyright y licencia [cite: 2026-02-01]
HEADER="/* ZMC v1.1 - (c) 2026 Volney Torres - Licensed under GNU GPLv3 */"

# Lista de extensiones a proteger [cite: 2026-01-31]
FILES=$(ls *.c *.h)

echo "Protegiendo archivos de ZMC..."

for file in $FILES
do
    # Verificar si ya tiene el encabezado para no duplicarlo [cite: 2026-02-01]
    if ! grep -q "Volney Torres" "$file"; then
        # Crear archivo temporal con el encabezado y el contenido original [cite: 2026-01-31]
        echo "$HEADER" | cat - "$file" > temp && mv temp "$file"
        echo " [+] $file: Encabezado de protección añadido."
    else
        echo " [!] $file: Ya contiene información de autoría."
    fi
done

echo "Proceso finalizado. Tus fuentes están protegidas legalmente."
