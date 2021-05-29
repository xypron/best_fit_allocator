
all:
	gcc alloc.c main.c -g -o alloc

check: all
	./alloc

clean:
	rm -f alloc
