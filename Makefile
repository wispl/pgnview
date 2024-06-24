CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined

# chess related object files, no gui
CHESS_OBJ = bitboard.o board.o movegen.o pgn.o pgn_ext.o
OBJECTS = $(CHESS_OBJ) termbox2.o

TESTS := $(wildcard tests/test_*.c)

pgncat: $(OBJECTS) main.c
	$(CC) -o pgncat main.c $(OBJECTS) $(LDFLAGS)

pgn_ext.o: pgn_ext.h board.h move.h movegen.h pgn.h
movegen.o: movegen.h bitboard.h board.h move.h
board.o: board.h bitboard.h move.h
pgn.o: pgn.h
bitboard.o: bitboard.h
termbox2.o: termbox2.h

.Phony: clean test $(TESTS)

clean:
	@rm -f pgncat test_* $(OBJECTS)

test: $(TESTS)

# TODO: use something else besides running and scanning for assertions
# TODO: cache results? keep executable around?
$(TESTS): tests/test_%.c: $(CHESS_OBJ)
	@$(CC) $@ -o $(basename $(notdir $@)) $(CHESS_OBJ) $(LDFLAGS)
	./$(basename $(notdir $@))
