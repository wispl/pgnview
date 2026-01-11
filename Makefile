CC = cc

CFLAGS += -Wextra -Wall -Wdouble-promotion
pgnview test: CFLAGS += -fsanitize=address,undefined -g3
release: CFLAGS += -O2 -g

LDFLAGS += -g
pgnview test: LDFLAGS += -fsanitize=address,undefined -g3
release: LDFLAGS += -g

CHESS_OBJS = bitboard.o board.o movegen.o
OBJS = $(CHESS_OBJS) pgn.o engine.o termbox2.o main.o
EXE = pgnview

RELEASE_DIR = release
RELEASE_EXE = $(RELEASE_DIR)/$(EXE)

TEST_DIR  = tests
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_EXES = $(patsubst %.c,%,$(TEST_SRCS))
TESTS     = $(wildcard $(TEST_DIR)/*.test)

TEST_RESULTS = $(addsuffix .res, $(TESTS))

.Phony: all
all: pgnview

pgnview: $(OBJS)
	$(CC) -o $(EXE) $(OBJS) $(LDFLAGS)

main.o: termbox2.h chess.h pgn.h
engine.o: chess.h pgn.h
termbox2.o: termbox2.h
pgn.o: pgn.h
$(CHESS_OBJS): chess.h

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

$(TEST_DIR)/%: $(TEST_DIR)/%.c $(PGN_OBJS) $(CHESS_OBJS)
	$(CC) $< $(CFLAGS) $(LDFLAGS) $(PGN_OBJS) $(CHESS_OBJS) -o $@

$(TEST_DIR)/%.test.res: $(TEST_DIR)/%.test
	./$<

test: $(TEST_EXES) $(TEST_RESULTS)
