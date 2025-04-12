SRC = bicycle.c bicycle_main.c
OBJ = $(SRC:.c=.o)
BIN = bicycle

CC = gcc
CFLAGS = -Wall -Wextra -Werror -fsanitize=address

all: $(BIN)

run: $(BIN)
	./$(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c bicycle.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
