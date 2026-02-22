
zmc.com: main.c panel.c operations.c globals.c zmc.h Makefile
	zcc +cpm -O3 -vn -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall \
	main.c panel.c operations.c globals.c -o zmc.com -create-app
