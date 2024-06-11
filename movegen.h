#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "move.h"

// move generate configuration
struct movegenc {
	enum movetype movetype;	// type of move to generate
	enum piece piece;	// piece to generate moves for
	enum color color;	// color to generate moves for
	u64 target;		// target bitboard
};

void init_lineattacks_table();
void generate_moves(struct board *board, struct movelist *moves, struct movegenc *conf);
#endif
