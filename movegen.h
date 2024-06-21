#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "move.h"

// Parameters to passed into generate_moves
struct movegenc {
	enum movetype movetype;	// type of move to generate
	enum piece piece;	// piece to generate moves for
	enum color color;	// color to generate moves for
	u64 target;		// target bitboard
};

void init_lineattacks_table();
struct move* generate_moves(struct board *board, struct move *moves, struct movegenc *conf);
#endif
