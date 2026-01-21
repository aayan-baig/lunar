CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

BIN = lunar
SRC = \
  src/main.c \
  src/diag.c \
  src/util.c \
  src/lexer.c \
  src/ast.c \
  src/parser.c

OBJ = $(SRC:.c=.o)

all: $(BIN)

$(BIN):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BIN) $(OBJ)


.PHONY: all clean