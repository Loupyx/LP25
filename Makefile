CC = gcc
CFLAGS = -Wall -Wextra -O2
LDLIBS = -lncurses	
EXEC = projet
SRC = main.c Processus.c key_detector.c
OBJ = $(SRC:.c=.o)

all: $(EXEC) clean

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

%.o: %.c Processus.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
