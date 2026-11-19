CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Isrc

#flag -I src  allows files not in src to simply have #include "lexer.h"
# without having to use #include "../src/lexer"
# (writing this here because i did not know)

# output file name
TARGET = ifj26

# all source files from src/
SRC = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)
OBJ = $(SRC:.c=.o)

# 'make' 'make all'
# compiles everything from src/ 
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@


# 'make clean' 'make cleanup'
# cleans up compiled files
clean:
	rm -f $(TARGET) $(OBJ) test_lexer_bin

cleanup: clean


# 'make test_lexer'
# runs lexer unit tests, cleans up afterwards
test_lexer:
	$(CC) $(CFLAGS) -g tests/test_lexer.c src/lexer.c src/token.c -o test_lexer_bin
	./test_lexer_bin
	rm -f test_lexer_bin
test_lexer_valgrind:
	$(CC) $(CFLAGS) -g tests/test_lexer.c src/lexer.c src/token.c -o test_lexer_bin
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./test_lexer_bin
	rm -f test_lexer_bin
