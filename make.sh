#!/bin/bash
# Compilador z88dk para Z80 / CP/M
# -O3: Optimización máxima
# -create-app: Genera el archivo .COM directamente

zcc +cpm -O3 -vn -DAMALLOC main.c panel.c operaciones.c globals.c -o zmc.com -create-app

if [ $? -eq 0 ]; then
    echo "✅ Compilación exitosa: ZMC.COM generado."
    # Si usas emulador, aquí podrías añadir la inyección al .dsk
    # cpmcp -f connor z80.dsk ZMC.COM 0:
else
    echo "❌ Error en la compilación."
fi
