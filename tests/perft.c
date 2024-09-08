#include "../chess.h"

#include <stdlib.h>
#include <stdio.h>

static struct board board;
static enum color color = WHITE;

u64 perft(int depth)
{
	if (depth == 0)
		return 1ULL;

	u64 nodes = 0;

	move moves[256];
	move *last = generate_legal_moves(&board, moves, color);
	color = flip_color(color);
	int n_moves = last - moves;

	for (int i = 0; i < n_moves; ++i) {
		enum piece_id captured_piece = board.squares[move_to(moves[i])];
		board_move(&board, moves[i]);
		nodes += perft(depth - 1);
		board_undo_move(&board, moves[i], captured_piece);
	}
	return nodes;
}

int
main(int argc, char **argv)
{
	int depth = strtol(argv[1], NULL, 10);
	board_init(&board);
	init_lineattacks_table();
	u64 nodes = perft(depth);
	printf("%llu\n", nodes);
	return 1;
}
