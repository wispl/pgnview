CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=undefined,address
LDFLAGS += -fsanitize=undefined,address
OBJECTS = main.o parser.o movegen.o pgn_movelist.o bitboard.o board.o

pgncat: $(OBJECTS)
	$(CC) -o pgncat $(OBJECTS) $(LDFLAGS)
main.o: parser.h pgn_movelist.h movegen.h bitboard.h array.h move.h
parser.o: parser.h array.h
pgn_movelist.o: pgn_movelist.h array.h bitboard.h movegen.h move.h
movegen.o: movegen.h board.h array.h move.h
board.o: board.h bitboard.h move.h
bitboard.o: bitboard.h
clean:
	rm -f pgncat $(OBJECTS)
