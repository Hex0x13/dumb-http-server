CC := gcc
FLAGS := -Wall -std=c99
SRC := src/main.c
DEST := dest/main

all: $(DEST)

$(DEST): $(SRC)
	$(CC) $(FLAGS) $(SRC) -o $(DEST)

clean:
	rm -f $(DEST)
