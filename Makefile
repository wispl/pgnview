CC = cc

CFLAGS += -Wextra -Wall -Wdouble-promotion
pgnview test: CFLAGS += -fsanitize=address,undefined -g3
release: CFLAGS += -O2 -g

LDFLAGS += -g
pgnview test: LDFLAGS += -fsanitize=address,undefined -g3
release: LDFLAGS += -g

CHESS_OBJS = bitboard.o board.o movegen.o
PGN_OBJS = pgn.o pgn_ext.o
OBJS = $(CHESS_OBJS) $(PGN_OBJS) termbox2.o main.o
EXE = pgnview

RELEASE_DIR = release
RELEASE_EXE = $(RELEASE_DIR)/$(EXE)

TESTS := $(wildcard tests/test_*.c)

.Phony: all
all: pgnview

pgnview: $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

main.o: termbox2.h chess.h pgn.h pgn_ext.h
$(CHESS_OBJS): chess.h
$(PGN_OBJS): pgn.h
pgn_ext.o: pgn_ext.h chess.h
termbox2.o: termbox2.h

release: mkdir $(RELEASE_EXE)

$(RELEASE_EXE): $(addprefix $(RELEASE_DIR)/, $(OBJS))
	$(CC) -o $(RELEASE_EXE) $^ $(LDFLAGS)

$(RELEASE_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

mkdir:
	@mkdir -p $(RELEASE_DIR)

.Phony: clean
clean:
	rm -rf $(RELEASE_DIR) $(EXE) test_* $(OBJS)

.Phony: test $(TESTS)
test: $(TESTS)

# TODO: use something else besides running and scanning for assertions
# TODO: cache results? keep executable around?
$(TESTS): tests/test_%.c: $(PGN_OBJS) $(CHESS_OBJS)
	@$(CC) $@ -o $(basename $(notdir $@)) $(PGN_OBJS) $(CHESS_OBJS) $(LDFLAGS)
	./$(basename $(notdir $@))
