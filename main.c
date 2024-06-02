#include "list.h"
#include "parser.h"

#include <stdio.h>

int main(void)
{
	struct pgn *pgn = pgn_read("test.pgn");
	struct tag *tag;
	list_for_each_entry(tag, &pgn->tags, node) {
		printf("%s: %s\n", tag->type, tag->description);
	}
	struct move *move;
	list_for_each_entry(move, &pgn->moves, node) {
		printf("%s %s\n", move->white, move->black);
	}

	return 1;
}
