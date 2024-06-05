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

enum lineattacks {
	DIAGONAL,	
	ANTIDIAGONAL,
	HORIZONTAL,
	VERTICAL,
};

enum color {
	WHITE,
	BLACK,
	COLOR_MAX,
};
#define flip_color(color) ((color) ^ BLACK)

enum piece {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	ALL,
	PIECE_MAX,
};

enum piece_id {
	W_KING = KING, W_QUEEN, W_ROOK, W_BISHOP, W_KNIGHT, W_PAWN,
	B_KING,        B_QUEEN, B_ROOK, B_BISHOP, B_KNIGHT, B_PAWN,
	EMPTY
};
#define piece_color(id) ((id) < B_KING)
#define piece_type(id)  ((id) - (piece_color((id)) * B_KING))

struct board {
	// index squares for piece id
	enum piece_id squares[64];
	// piece bitboards
	u64 pieces[PIECE_MAX];
	// color bitboards
	u64 colors[COLOR_MAX];
};
#define pieces(board, piece, color) ((board)->pieces[(piece)] & (board)->colors[(color)])
#define pawns(board, color) (pieces((board), PAWN, (color)))

enum movetype {
	QUIET,		// movement only, no material is altered
	PROMOTION,	// material is added for promoting side
	CAPTURE,	// material is decreased for opposing side
};

struct move {
	int from;
	int to;
};

struct movelist {
	struct move moves[256];
	int len;
};

void init_lineattacks_table();
void movelist_clear(struct movelist *list);
void generate_moves(struct board *board, struct movelist *list, enum piece piece,
		    enum color color, enum movetype type);

void board_init(struct board *board);
void board_add(struct board *board, int square, enum piece_id id);
void board_remove(struct board *board, int square);
void board_move(struct board *board, struct move *move);
void board_print(struct board *board);

#endif
