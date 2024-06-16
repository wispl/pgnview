#ifndef BOARD_H
#define BOARD_H

#include "bitboard.h"
#include "move.h"

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
	EMPTY,
	PIECE_ID_MAX
};
#define piece_color(id) (!((id) < B_KING))
#define piece_type(id)  ((id) - (piece_color((id)) * B_KING))

enum castling {
	// 0000
	NO_CASTLING,
	// 0001
	WHITE_KINGSIDE  = 1,
	// 0010
	WHITE_QUEENSIDE = WHITE_KINGSIDE << 1,
	// 0011
	WHITE_BOTHSIDE  = WHITE_KINGSIDE | WHITE_QUEENSIDE,
	// 0100
	BLACK_KINGSIDE  = WHITE_KINGSIDE << 2,
	// 1000
	BLACK_QUEENSIDE = WHITE_KINGSIDE << 3,
	// 1100
	BLACK_BOTHSIDE  = BLACK_KINGSIDE | BLACK_QUEENSIDE,
	// 1111
	ANY_CASTLING = WHITE_BOTHSIDE | BLACK_BOTHSIDE
};

struct board {
	// index squares for piece id
	enum piece_id squares[64];
	// piece bitboards
	u64 pieces[PIECE_MAX];
	// color bitboards
	u64 colors[COLOR_MAX];
	// castling rights
	enum castling castling;
};

#define pieces(board, piece, color) ((board)->pieces[(piece)] & (board)->colors[(color)])
#define pawns(board, color) (pieces((board), PAWN, (color)))
#define can_castle_short(board, color) ((board)->castling & (WHITE_KINGSIDE << 2 * (color)))
#define can_castle_long(board, color)  ((board)->castling & (WHITE_QUEENSIDE << 2 * (color)))

void board_init(struct board *board);
void board_put_piece(struct board *board, int square, enum piece_id id);
void board_del_piece(struct board *board, int square);
void board_move_piece(struct board *board, int from, int to);
void board_move(struct board *board, struct move *move);
void board_print(struct board *board);

#endif
