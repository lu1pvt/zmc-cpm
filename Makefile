ZCC := $(shell command -v zcc 2>/dev/null)
ifndef ZCC
ZCC := docker run --rm -v $(CURDIR):/src -w /src z88dk/z88dk zcc
endif

zmc.com: main.c panel.c operations.c globals.c zmc.h Makefile
	$(ZCC) +cpm -O3 -vn -DAMALLOC -pragma-define:CRT_STACK_SIZE=1024 -Wall \
	main.c panel.c operations.c globals.c -o zmc.com -create-app
