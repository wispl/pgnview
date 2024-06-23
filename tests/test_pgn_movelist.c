#include "../array.h"
#include "../movegen.h"
#include "../pgn.h"
#include "../pgn_movelist.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

static inline int ranktoi(char c)
{
	return (c - '1') * 8;
}

static inline int filetoi(char c)
{
	return c - 'a';
}

static inline int santoi(char *san)
{
	return filetoi(san[0]) + ranktoi(san[1]);
}

// Testing "from square" is too tedious and error prone and is better suited
// visually then textually, so skip testing it. Testing "to square" should be
// enough to determine correctless as it isolates error to disambiguation code.
void test_pgn_movelist(char *file, char* orig[], char* to[])
{
	struct pgn pgn;
	pgn_read(&pgn, file);

	struct move *moves = malloc(sizeof(moves[0]) * pgn.moves.len);
	int moves_len = pgn_to_moves(&pgn.moves, moves);

	assert(pgn.moves.len == moves_len && "Moves ommited");

	// TODO: expand, promotion, castle, etc..
	struct move move;
	for (int i = 0; i < moves_len; ++i) {
		move = moves[i];
		if (strchr(orig[i], 'x')) {
			assert(move.movetype == CAPTURE && "Wrong movetype");
		}
		assert((move.to == santoi(to[i])) && "To square mismatch");
	}

	free(moves);
	pgn_free(&pgn);
}

int main(void)
{
	init_lineattacks_table();
	char *orig[] = {
		"e4", "e5", "Nf3", "d6", "d4", "exd4", "Nxd4", "Nc6", "Nc3", "a6",
		"Bc4", "h6", "O-O", "Nge7", "Be3", "Ng6", "Qf3", "Nce5", "Qe2", "Nxc4",
		"Qxc4", "Ne5", "Qe2", "Bd7", "h3", "Qc8", "f4", "Ng6", "f5", "Ne5",
		"Nf3", "Be7", "Nxe5", "dxe5", "Qg4", "Kf8", "Rad1", "Bf6", "Bc5+",
		"Kg8", "Nd5", "b6", "Nxf6#"
	};
	char *to[] = {
		"e4", "e5", "f3", "d6", "d4", "d4", "d4", "c6", "c3", "a6",
		"c4", "h6", "h1", "e7", "e3", "g6", "f3", "e5", "e2", "c4",
		"c4", "e5", "e2", "d7", "h3", "c8", "f4", "g6", "f5", "e5",
		"f3", "e7", "e5", "e5", "g4", "f8", "d1", "f6", "c5",
		"g8", "d5", "b6", "f6"
	};
	test_pgn_movelist("tests/test.pgn", orig, to);
}
