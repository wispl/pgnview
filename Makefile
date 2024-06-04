CC = cc
CFLAGS = -Wextra -Wall -Wdouble-promotion -g3
OBJECTS = main.o parser.o movegen.o bitboard.o

pgncat: $(OBJECTS)
	$(CC) -o pgncat $(OBJECTS)
main.o: parser.h list.h
parser.o: parser.h list.h
movegen.o: movegen.h list.h
bitboard.o: bitboard.h
clean:
	rm -f pgncat $(OBJECTS)
