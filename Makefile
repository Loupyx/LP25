CC = gcc
CFLAGS = -Wall -Wextra -O2
EXEC = projet

all: $(EXEC) clean

$(EXEC): main.o
	$(CC) -o $@ $^

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o
