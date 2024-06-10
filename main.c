#include "array.h"
#include "bitboard.h"
#include "movegen.h"
#include "pgn_movelist.h"
#include "parser.h"

#include <stdio.h>

static void test_parser()
{
	printf("============= Testing Parser =============\n");
	struct pgn pgn;

	pgn_read(&pgn, "test.pgn");
	for (int i = 0; i < pgn.tags.len; ++i) {
		struct pgn_tag tag = array_get(&pgn.tags, i);
		printf("%s \"%s\"\n", tag.name, tag.desc);
	}
	printf("\n");
	for (int i = 0; i < pgn.moves.len; ++i) {
		printf("%s ", array_get(&pgn.moves, i).text);
	}
	printf("%s\n\n", pgn.result);

	pgn_free(&pgn);
	pgn_read(&pgn, "test2.pgn");
	for (int i = 0; i < pgn.tags.len; ++i) {
		struct pgn_tag tag = array_get(&pgn.tags, i);
		printf("%s \"%s\"\n", tag.name, tag.desc);
	}
	printf("\n");
	for (int i = 0; i < pgn.moves.len; ++i) {
		printf("%s ", array_get(&pgn.moves, i).text);
	}
	printf("%s\n", pgn.result);
	pgn_free(&pgn);
}

static void test_movegen()
{
	printf("============= Testing Movegen =============\n");
	init_lineattacks_table();
	struct movelist ARRAY(moves);
	struct board board;
	board_init(&board);

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
	array_free(&moves);
}

static void test_board()
{
	printf("============= Testing Board =============\n");
	struct board board;
	board_init(&board);

	struct move m0 = {
		.movetype = QUIET,
		.from = e2,
		.to   = e4
	};
	struct move m1 = {
		.movetype = QUIET,
		.from = g8,
		.to   = f6
	};
	board_move(&board, &m0);
	board_move(&board, &m1);

	board_print(&board);
}

static void test_pgn_movelist()
{
	printf("============= Testing PGN Movelist =============\n");
	struct pgn pgn;
	struct movelist ARRAY(moves);

	pgn_read(&pgn, "test.pgn");
	pgn_movelist(&pgn.moves, &moves);

	for (int i = 0; i < moves.len; ++i) {
		struct move move = array_get(&moves, i);
		print_square(move.from);
		printf("->");
		print_square(move.to);
		printf(" ");
	}
	printf("\n");
	array_free(&moves);
	pgn_free(&pgn);
}

int main(int argc, char **argv)
{
	test_parser();
	test_movegen();
	test_board();
	test_pgn_movelist();
	return 1;
}
