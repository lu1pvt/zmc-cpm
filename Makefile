all: zmc.com zmc8080.com keytest.com

zmc.com: main.c panel.c operations.c globals.c vt100.c zmc.h Makefile
	zcc +cpm -O3 -vn -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall \
	main.c panel.c operations.c globals.c vt100.c -o zmc.com -create-app

zmc8080.com: main.c panel.c operations.c globals.c vt100.c zmc.h Makefile
	zcc +cpm -clib=8080 -O3 -vn -Di8080 -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall \
	main.c panel.c operations.c globals.c vt100.c -o zmc8080.com -create-app

keytest.com: keytest.c Makefile
	zcc +cpm -O3 -vn -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall \
	keytest.c -o keytest.com -create-app
