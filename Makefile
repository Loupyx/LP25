CC = gcc
CFLAGS = -Wall -Wextra -O2
LDLIBS = -lncurses -lssh
EXEC = projet
SRC := $(wildcard *.c */*.c)
OBJ = $(SRC:.c=.o)

all: $(EXEC) clean

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o */*.o
