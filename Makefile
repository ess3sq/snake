CC=gcc
SOURCES=snake.c
CCFLAGS=-ggdb -Wall -Wextra -Werror -pedantic
LDFLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

.PHONY: clean

snake: $(SOURCES)
	$(CC) -o snake $(SOURCES) $(LDFLAGS) $(CCFLAGS)

clean:
	rm snake
