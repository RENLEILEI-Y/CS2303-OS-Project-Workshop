SRC = lcm.c lcm_main.c
OBJ = $(SRC:.c=.o)
BIN = lcm

CC = gcc
CFLAGS = -Wall -Wextra -Werror -fsanitize=address

all: $(BIN)

run: $(BIN)
	./$(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c lcm.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
