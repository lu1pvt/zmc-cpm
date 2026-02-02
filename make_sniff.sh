#!/bin/bash
zcc +cpm -O3 -vn sniffer.c -o SNIFF.COM -create-app
if [ $? -eq 0 ]; then
    echo "✅ SNIFF.COM generado."
else
    echo "❌ Error."
fi
