#include "list.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc > 3 || argc < 2) {
		printf("[Error] No arguments or too many arguments were passed");
		return 0;
	}

	struct pgn *pgn = malloc(sizeof(struct pgn));
	pgn_read(pgn, argv[1]);

	 struct tag *tag;
	 list_for_each_entry(tag, &pgn->tags, node) {
	 	printf("%s: %s\n", tag->name, tag->desc);
	 }

	 struct move *move;
	 list_for_each_entry(move, &pgn->moves, node) {
	 	printf("%s %s\n", move->white, move->black);
	 }
	return 1;
}
