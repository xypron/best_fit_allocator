
all:
	$(CROSS_COMPILE)gcc alloc.c main.c -g -Wall -Wextra -o alloc

check: all
	./alloc

clean:
	rm -f alloc
