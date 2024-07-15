CC = cc
CFLAGS += -Wextra -Wall -Wdouble-promotion
LDFLAGS += -g

# chess related object files, no gui
CHESS_OBJS = bitboard.o types.o movegen.o pgn.o pgn_ext.o
OBJS = $(CHESS_OBJS) termbox2.o main.o
EXE = pgnview

RELEASE_DIR = release
RELEASE_EXE = $(RELEASE_DIR)/$(EXE)

TESTS := $(wildcard tests/test_*.c)

all: CFLAGS += -fsanitize=address,undefined -g3
all: LDFLAGS += -fsanitize=address,undefined -g3
all: pgnview

pgnview: $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

main.o: termbox2.h types.h pgn.h pgn_ext.h
pgn_ext.o: pgn_ext.h types.h movegen.h pgn.h
movegen.o: movegen.h types.h
types.o: types.h
pgn.o: pgn.h
bitboard.o: types.h
termbox2.o: termbox2.h

release: CFLAGS += -O2 -g
release: LDFLAGS += -g
release: mkdir $(RELEASE_EXE)

$(RELEASE_EXE): $(addprefix $(RELEASE_DIR)/, $(OBJS))
	$(CC) -o $(RELEASE_EXE) $^ $(LDFLAGS)

$(RELEASE_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

mkdir:
	@mkdir -p $(RELEASE_DIR)

.Phony: clean test $(TESTS)

clean:
	rm -rf $(RELEASE_DIR) $(EXE) test_* $(OBJS)

test: $(TESTS)

# TODO: use something else besides running and scanning for assertions
# TODO: cache results? keep executable around?
$(TESTS): tests/test_%.c: $(CHESS_OBJS)
	@$(CC) $@ -o $(basename $(notdir $@)) $(CHESS_OBJS) $(LDFLAGS)
	./$(basename $(notdir $@))
