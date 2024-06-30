#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "types.h"

enum gentype {
	QUIET,
	CASTLE,
	CAPTURE,
	PROMOTION,
	PROMO_CAPTURE
};

// Parameters to passed into generate_moves
struct movegenc {
	enum gentype type;	// type of move to generate
	enum piece piece;	// piece to generate moves for
	enum color color;	// color to generate moves for
	u64 target;		// targets bitboard
};

bool attacks_table_initilized();
void init_lineattacks_table();
move* generate_moves(struct board *board, move *moves, struct movegenc *conf);

#endif
