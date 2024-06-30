CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion -g3 -fsanitize=address,undefined
LDFLAGS += -fsanitize=address,undefined

# chess related object files, no gui
CHESS_OBJS = bitboard.o board.o movegen.o pgn.o pgn_ext.o
OBJS = $(CHESS_OBJS) termbox2.o main.o

TESTS := $(wildcard tests/test_*.c)

pgncat: $(OBJS)
	$(CC) -o pgncat $(OBJS) $(LDFLAGS)

main.o: termbox2.h board.h move.h movegen.h pgn.h pgn_ext.h
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
$(TESTS): tests/test_%.c: $(CHESS_OBJS)
	@$(CC) $@ -o $(basename $(notdir $@)) $(CHESS_OBJS) $(LDFLAGS)
	./$(basename $(notdir $@))
