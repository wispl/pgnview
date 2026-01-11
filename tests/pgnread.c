#include "../pgn.h"

#include <stdio.h>

// This takes in a pgn file in import format and then prints it out in export
// format. The difference between the two is that import format has lax
// constaints but export format is strict. This is a good way to test our
// pgn parser as well since there is a standardized format.
int main(int argc, char **argv) 
{
	if (argc < 2) {
		fprintf(stderr, "Please specify a file!\n");
		return 0;
	}

	struct pgn pgn;

	enum pgn_result result =  pgn_read(&pgn, argv[1]);
	if (result != PGN_OK) 
		printf("Error at reader: error code %d\n", result);

	for (int i = 0; i < vec_len(pgn.games); ++i) {
		struct pgn_game *game = &pgn.games[i];
		if (game->result != PGN_OK) {
			printf("Error at game #%d: error code %d\n", i, game->result);
			continue;
		}
		pgn_print_game(game);
	}

	pgn_free(&pgn);
	return 1;
}
