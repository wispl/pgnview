#ifndef BOARD_H
#define BOARD_H

#include "list.h"

typedef unsigned long long u64;

#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (bitboard &= ~(1ULL << square))

#define file_a 0x0101010101010101ULL
#define file_b (file_a << 1)
#define file_c (file_a << 2)
#define file_d (file_a << 3)
#define file_e (file_a << 4)
#define file_f (file_a << 5)
#define file_g (file_a << 6)
#define file_h (file_a << 7)

#define rank_1 0xFFULL
#define rank_2 (rank_1 << (8 * 1))
#define rank_3 (rank_1 << (8 * 2))
#define rank_4 (rank_1 << (8 * 3))
#define rank_5 (rank_1 << (8 * 4))
#define rank_6 (rank_1 << (8 * 5))
#define rank_7 (rank_1 << (8 * 6))
#define rank_8 (rank_1 << (8 * 7))

// shifts
#define north(b)      ((b << 8))
#define south(b)      ((b >> 8))

#define east(b)       ((b << 1) & ~file_a)
#define north_east(b) ((b << 9) & ~file_a)
#define south_east(b) ((b >> 7) & ~file_a)

#define west(b)       ((b >> 1) & ~file_h)
#define north_west(b) ((b << 7) & ~file_h)
#define south_west(b) ((b >> 9) & ~file_h)

#define pos_ray(ray, sq) ((ray) & (-2ULL << (sq)))
#define neg_ray(ray, sq) ((ray) & ((1ULL << (sq)) - 1))

enum color { WHITE, BLACK, BOTH };

enum piece {
	KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN
};

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

enum direction {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
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

struct move {
	struct node node;
	enum movetype type;
	int from;
	int to;
};

struct board {
	// all pieces:
	// pieces[white|black][king|queen|rook|bishop|knight|pawn]
	u64 pieces[2][6];
	// occupied squares:
	// occupied[white|black|both]
	u64 occupied[3];
};

#endif
