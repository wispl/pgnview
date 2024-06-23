CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined

NO_GUI = pgn.o movegen.o pgn_movelist.o bitboard.o board.o
OBJECTS = $(NO_GUI) termbox2.o

TESTS := $(wildcard tests/test_*.c)

pgncat: $(OBJECTS) main.c
	$(CC) -o pgncat main.c $(OBJECTS) $(LDFLAGS)

pgn.o: pgn.h array.h
pgn_movelist.o: pgn_movelist.h array.h movegen.h move.h
movegen.o: movegen.h board.h array.h move.h
board.o: board.h bitboard.h move.h
bitboard.o: bitboard.h
termbox2.o: termbox2.h

.Phony: clean test $(TESTS)

clean:
	@rm -f pgncat test_* $(OBJECTS)

test: $(TESTS)

# TODO: use something else besides running and scanning for assertions
# TODO: cache results? keep executable around?
$(TESTS): tests/test_%.c: pgn_movelist.h array.h bitboard.h movegen.h move.h
	@$(CC) $@ -o $(basename $(notdir $@)) $(NO_GUI) $(LDFLAGS)
	./$(basename $(notdir $@))
