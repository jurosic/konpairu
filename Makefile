
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I src

#flag -I src  allows files not in src to simply have #include "lexer.h"
# without having to use #include "../src/lexer"
# (writing this here because i did not know)

# output file name
TARGET = ifj26

# all source files from src/
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

# 'make' 'make all'
# compiles everything from src/ 
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


# 'make clean' 'make cleanup'
# cleans up compiled files
clean:
	rm -f $(TARGET) src/*.o

cleanup: clean


# 'make testlexer'
# runs lexer unit tests, cleans up afterwards
test_lexer:
	$(CC) $(CFLAGS) tests/test_lexer.c src/lexer.c src/token.c -o test_lexer
	./test_lexer
	rm -f test_lexer

