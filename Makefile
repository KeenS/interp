CFLAGS = -std=c89 -ggdb
LDFLAGS =
OBJS = main.o vm.o

default: all

.PHONY: all run default test clean

all: main

run: main
	./main

main: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS)

vm.o: vm.c vm.h stack.h
	$(CC) -o $@ $(CFLAGS) -c $<

main.o: main.c vm.h
	$(CC) -o $@ $(CFLAGS) -c $<

vm.h: stack.h

clean:
	rm *.o
