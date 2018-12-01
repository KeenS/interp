CFLAGS = -std=c89 -ggdb -O3 -Wall -Wextra
LDFLAGS =
OBJS =

default: all

.PHONY: simple threaded direct_threaded simple_jit default clean

all: simple threaded direct_threaded simple_jit

simple: main_simple
	time ./main_simple

threaded: main_threaded
	time ./main_threaded

direct_threaded: main_direct_threaded
	time ./main_direct_threaded

simple_jit: main_simple_jit
	time ./main_simple_jit

main_simple: main.o vm_simple.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) main.o vm_simple.o

main_threaded: main.o vm_threaded.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) main.o vm_threaded.o

main_direct_threaded: main.o vm_direct_threaded.o
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) main.o vm_direct_threaded.o

main_simple_jit: main.o vm_simple_jit.o
	$(CC) -o $@  $(CFLAGS) $(LDFLAGS) -std=gnu  main.o vm_simple_jit.o


vm_%.o: vm_%.c vm.h stack.h
	$(CC) -o $@ $(CFLAGS) -c $<

main.o: main.c vm.h
	$(CC) -o $@ $(CFLAGS) -c $<

vm.h: stack.h

clean:
	rm -f *.o
	rm -f main_simple main_threaded main_direct_threaded
