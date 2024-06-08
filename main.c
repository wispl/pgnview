#include "bitboard.h"
#include "list.h"
#include "movegen.h"
#include "parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc > 3 || argc < 2) {
		printf("[Error] No arguments or too many arguments were passed");
		return 0;
	}

	printf("============= Testing Parser =============\n");
	struct pgn pgn;
	pgn_read(&pgn, argv[1]);

	struct pgn_tag *tag;
	list_for_each_entry(tag, &pgn.tags, node) {
		printf("%s: %s\n", tag->name, tag->desc);
	}
	struct pgn_move *move;
	list_for_each_entry(move, &pgn.moves, node) {
		printf("%s ", move->text);
	}
	printf("%s\n", pgn.result);

	pgn_free(&pgn);
	printf("\n");

	printf("============= Testing Movegen =============\n");
	init_lineattacks_table();
	struct board board;
	board_init(&board);

	struct movelist movelist;
	struct movegenc movegenc = {
		.piece = PAWN,
		.color = WHITE,
		.movetype = QUIET
	};
	generate_moves(&board, &movelist, &movegenc);

	for (int i = 0; i < movelist.len; ++i) {
		struct move move = movelist.moves[i];
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}
	printf("\n");

	printf("\n");

	printf("============= Testing Board =============\n");
	board_move(&board, &movelist.moves[2]);
	struct move knight_move = {
		.type = QUIET,
		.from = g8,
		.to   = f6
	};
	board_move(&board, &knight_move);
	board_print(&board);

	return 1;
}
