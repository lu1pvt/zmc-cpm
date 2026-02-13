
zmc.com: main.c panel.c operaciones.c globals.c zmc.h
	zcc +cpm -O3 -vn -DAMALLOC main.c panel.c operaciones.c globals.c -o zmc.com -create-app
