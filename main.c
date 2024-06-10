#include "array.h"
#include "bitboard.h"
#include "movegen.h"
#include "pgn_movelist.h"
#include "parser.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc > 3 || argc < 2) {
		printf("[Error] No arguments or too many arguments were passed\n");
		return 0;
	}

	printf("============= Testing Parser =============\n");
	struct pgn pgn;
	pgn_read(&pgn, argv[1]);

	for (int i = 0; i < pgn.tags.len; ++i) {
		printf("%s ", array_get(&pgn.tags, struct pgn_tag, i).name);
		printf("%s\n", array_get(&pgn.tags, struct pgn_tag, i).desc);
	}
	printf("\n");
	for (int i = 0; i < pgn.moves.len; ++i) {
		printf("%s ", array_get(&pgn.moves, struct pgn_move, i).text);
	}
	printf("%s\n", pgn.result);

	printf("\n");
	
	pgn_free(&pgn);

	printf("============= Testing Movegen =============\n");
	init_lineattacks_table();
	struct board board;
	board_init(&board);

	struct array moves;
	array_init(&moves, sizeof(struct move));
	struct movegenc movegenc = {
		.piece = PAWN,
		.color = WHITE,
		.movetype = QUIET,
		.target   = ~(0ULL),
	};
	generate_moves(&board, &moves, &movegenc);

	for (int i = 0; i < moves.len; ++i) {
		struct move move = array_get(&moves, struct move, i);
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}
	printf("\n\n");

	printf("============= Testing Board =============\n");
	board_move(&board, &array_get(&moves, struct move, 2));
	struct move knight_move = {
		.movetype = QUIET,
		.from = g8,
		.to   = f6
	};
	board_move(&board, &knight_move);
	board_print(&board);

	array_free(&moves);

	printf("============= Testing PGN Movelist =============\n");
	struct array movelist;
	array_init(&movelist, sizeof(struct move));
	pgn_movelist(&pgn.moves, &movelist);
	for (int i = 0; i < movelist.len; ++i) {
		struct move move = array_get(&movelist, struct move, i);
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}

	return 1;
}
