CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=undefined,address
LDFLAGS += -fsanitize=undefined,address
OBJECTS = parser.o movegen.o pgn_movelist.o bitboard.o board.o termbox2.o

pgncat: $(OBJECTS) main.c
	$(CC) -o pgncat main.c $(OBJECTS) $(LDFLAGS)

# TODO: better testing
test: $(OBJECTS) tests/test.c
	$(CC) -o test tests/test.c $(OBJECTS) $(LDFLAGS)
# test.o: parser.h pgn_movelist.h movegen.h bitboard.h array.h move.h

parser.o: parser.h array.h
pgn_movelist.o: pgn_movelist.h array.h bitboard.h movegen.h move.h
movegen.o: movegen.h board.h array.h move.h
board.o: board.h bitboard.h move.h
bitboard.o: bitboard.h
termbox2.o: termbox2.h

.Phony: clean
clean:
	rm -f pgncat test $(OBJECTS)
