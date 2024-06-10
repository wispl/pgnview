CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=undefined,address
LDFLAGS += -fsanitize=undefined,address
OBJECTS = main.o parser.o movegen.o pgn_movelist.o bitboard.o

pgncat: $(OBJECTS)
	$(CC) -o pgncat $(OBJECTS) $(LDFLAGS)
main.o: parser.h pgn_movelist.h movegen.h bitboard.h array.h
parser.o: parser.h array.h
pgn_movelist.o: pgn_movelist.h array.h bitboard.h movegen.h
movegen.o: movegen.h array.h bitboard.h
bitboard.o: bitboard.h
clean:
	rm -f pgncat $(OBJECTS)
