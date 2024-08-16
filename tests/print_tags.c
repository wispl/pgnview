#include "../pgn.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	struct pgn pgn;
	pgn_read(&pgn, argv[1]);

	for (int i = 0; i < pgn.tagcount; ++i)
		printf("%s\n", pgn.tags[i].desc);

	pgn_free(&pgn);
}
