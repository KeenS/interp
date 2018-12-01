CFLAGS = -std=c89 -ggdb -O3 -Wall -Wextra
LDFLAGS =
OBJS =

default: all

.PHONY: simple threaded run default test clean

all: simple threaded

simple: main_simple
	time ./main_simple

threaded: main_threaded
	time ./main_threaded

run: main_simple
	./main_simple

main_simple: main.o vm_simple.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) main.o vm_simple.o

main_threaded: main.o vm_threaded.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) main.o vm_threaded.o

vm_%.o: vm_%.c vm.h stack.h
	$(CC) -o $@ $(CFLAGS) -c $<

main.o: main.c vm.h
	$(CC) -o $@ $(CFLAGS) -c $<

vm.h: stack.h

clean:
	rm -f *.o
	rm -f main_simple
