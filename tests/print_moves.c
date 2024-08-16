#include "../pgn.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	struct pgn pgn;
	pgn_read(&pgn, argv[1]);

	for (int i = 0; i < pgn.movecount; ++i)
		printf("%s ", pgn.moves[i].text);

	pgn_free(&pgn);
	return 0;
}
