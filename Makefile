CC = cc
CFLAGS = -Wextra -Wall -Wdouble-promotion -g3
OBJECTS = main.o parser.o lexer.o

pgncat: $(OBJECTS)
	$(CC) -o pgncat $(OBJECTS)
main.o: parser.h list.h
parser.o: parser.h list.h lexer.h
lexer.o: lexer.h
clean:
	rm -f pgncat $(OBJECTS)
