CC = gcc
CFLAGS = -Wall -Wextra -O2
EXEC = projet
SRC = main.c Processus.c
OBJ = $(SRC:.c=.o)

all: $(EXEC) clean

$(EXEC): $(OBJ)
	$(CC) -o $@ $^

%.o: %.c Processus.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
