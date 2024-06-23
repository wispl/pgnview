#include "../array.h"
#include "../pgn.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define lengthof(a) (sizeof((a)) / sizeof((a)[0]))

void assert_str(char* s1, char *s2)
{
	assert(strcmp(s1, s2) == 0 && "Mismatch strings");
}

void test_pgn(char *file, char* tags[], int tagslen, char* moves[], int moveslen)
{
	struct pgn pgn;
	pgn_read(&pgn, file);

	assert(pgn.tags.len == tagslen && "Tags ommited");
	assert(pgn.moves.len == moveslen && "Moves ommited");
	assert_str(pgn.result, "1-0");

	char buffer[1024];
	struct pgn_tag tag;
	for (int i = 0; i < pgn.tags.len; ++i) {
		tag = array_get(&pgn.tags, i);
		sprintf(buffer, "%s %s\0", tag.name, tag.desc);
		assert_str(buffer, tags[i]);
	}

	for (int i = 0; i < pgn.moves.len; ++i) {
		assert_str(array_get(&pgn.moves, i).text, moves[i]);
	}

	pgn_free(&pgn);
}

int main(void)
{
	// TODO: Add more tests
	char *tags[] = {
		"Event Casual Classical game", "Site https://lichess.org/nvtiBwGP",
		"Date 2023.07.20", "White Anonymous", "Black Anonymous", "Result 1-0",
		"UTCDate 2023.07.20", "UTCTime 15:38:18", "WhiteElo ?", "BlackElo ?",
		"Variant Standard", "TimeControl 10800+180", "ECO C41",
		"Opening Philidor Defense: Exchange Variation", "Termination Normal"
	};
	char *moves[] = {
		"e4", "e5", "Nf3", "d6", "d4", "exd4", "Nxd4", "Nc6", "Nc3", "a6",
		"Bc4", "h6", "O-O", "Nge7", "Be3", "Ng6", "Qf3", "Nce5", "Qe2", "Nxc4",
		"Qxc4", "Ne5", "Qe2", "Bd7", "h3", "Qc8", "f4", "Ng6", "f5", "Ne5",
		"Nf3", "Be7", "Nxe5", "dxe5", "Qg4", "Kf8", "Rad1", "Bf6", "Bc5+",
		"Kg8", "Nd5", "b6", "Nxf6#"
	};
	test_pgn("tests/test.pgn", tags, lengthof(tags),
	         moves, lengthof(moves));
}
