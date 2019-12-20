BIN = test

CFLAGS = -Wall -Wextra -pedantic -Ofast -flto -march=native

LDFLAGS = -lm

CC = gcc

SRC = test.cc tinn.cc

all:
	$(CC) -o $(BIN) $(SRC) $(CFLAGS) $(LDFLAGS)

run:
	./$(BIN)

clean:
	rm -f $(BIN)
