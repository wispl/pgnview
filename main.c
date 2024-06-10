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
		printf("%s ", array_get(&pgn.tags, i).name);
		printf("%s\n", array_get(&pgn.tags, i).desc);
	}
	printf("\n");
	for (int i = 0; i < pgn.moves.len; ++i) {
		printf("%s ", array_get(&pgn.moves, i).text);
	}
	printf("%s\n", pgn.result);

	printf("\n");
	
	pgn_free(&pgn);

	printf("============= Testing Movegen =============\n");
	init_lineattacks_table();
	struct board board;
	board_init(&board);

	struct movelist moves;
	array_init(&moves);
	struct movegenc movegenc = {
		.piece = PAWN,
		.color = WHITE,
		.movetype = QUIET,
		.target   = ~(0ULL),
	};
	generate_moves(&board, &moves, &movegenc);

	for (int i = 0; i < moves.len; ++i) {
		struct move move = array_get(&moves, i);
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}
	printf("\n\n");

	printf("============= Testing Board =============\n");
	board_move(&board, &array_get(&moves, 2));
	struct move knight_move = {
		.movetype = QUIET,
		.from = g8,
		.to   = f6
	};
	board_move(&board, &knight_move);
	board_print(&board);

	array_free(&moves);

	printf("============= Testing PGN Movelist =============\n");
	struct movelist ARRAY(movelist);
	pgn_movelist(&pgn.moves, &movelist);
	for (int i = 0; i < movelist.len; ++i) {
		struct move move = array_get(&movelist, i);
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}

	return 1;
}
