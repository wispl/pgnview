#include "../pgn.h"

#include <stdio.h>

// TODO: merge with main program? seems pretty useful in general

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

	for (int i = 0; i < vec_len(pgn.games); ++i) {
		struct pgn_game *game = &pgn.games[i];
		if (game->result != PGN_OK) {
			printf("Error at game #%d: error code %d\n", i, game->result);
			continue;
		}

		for (int j = 0; j < vec_len(game->tags); ++j)
			printf("[%s \"%s\"]\n", game->tags[j].name, game->tags[j].desc);

		printf("\n");
		for (int k = 0; k < vec_len(game->plies); ++k) {
			if (k % 2 == 0) {
				printf("%d. ", (k / 2) + 1);
			}
			printf("%s ", game->plies[k].text);

			if (game->plies[k].nag > 0) {
				printf("$%d ", game->plies[k].nag);
			}

			if (game->plies[k].comment != NULL) {
				printf("{%s} ", game->plies[k].comment);
			}
		}
		switch (game->termination) {
		case DRAW: printf("1/2-1/2\n"); break;
		case WHITE_WIN: printf("1-0\n"); break;
		case BLACK_WIN: printf("0-1\n"); break;
		case UNKNOWN: printf("*\n"); break;
		}
		printf("\n");
	}

	pgn_free(&pgn);
	return 1;
}
