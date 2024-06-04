#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"

// Little endian ranked-file ordered square mapping
enum squares {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

enum color {
	WHITE,
	BLACK,
	BOTH
};

enum piece {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN
};;

struct board {
	// all pieces: pieces[white|black][king|queen|rook|bishop|knight|pawn]
	u64 pieces[2][6];
	// occupied squares: occupied[white|black|both]
	u64 occupied[3];
};

enum lineattacks {
	DIAGONAL,	
	ANTIDIAGONAL,
	HORIZONTAL,
	VERTICAL,
};

enum movetype {
	QUIET,		// movement only, no material is altered
	PROMOTION,	// material is added for promoting side
	CAPTURE,	// material is decreased for opposing side
};

// move encoding
struct move {
	enum movetype type;
	int from;
	int to;
};

struct movelist {
	struct move moves[256];
	int len;
};

void init_lineattacks_table();
void generate_moves(struct board *board, struct movelist *list, enum piece piece,
		    enum color color, enum movetype type);
void movelist_clear(struct movelist *list);

#endif
