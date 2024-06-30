#ifndef TYPES_H
#define TYPES_H

#include "bitboard.h"

#include <stdbool.h>
#include <stdint.h>

// 6 bits for from square
// 6 bits for to square
// 1 bit for capture flag
// 1 bit for promotion flag
// 2 bits for special flags (promo piece or castling)
typedef uint16_t move;

#define make_move(from, to, capture, promo, special) \
	((from) + ((to) << 6) + ((capture) << 12) + ((promo) << 13) + ((special) << 14))
#define make_quiet(from, to) (make_move((from), (to), false, false, 0))
#define make_capture(from, to) (make_move((from), (to), true, false, 0))
#define make_castle(from, to) (make_move((from), (to), false, false, true))
#define make_promotion(from, to, is_capture, piece) \
	(make_move((from), (to), (is_capture), true, (piece)))

#define move_from(move)          ((move) & 0x3f)
#define move_to(move)            (((move) >> 6) & 0x3f)
#define move_promo_piece(move)   (((move) >> 14) & 0x3)

#define move_set_from(move, from)         ((move) |= ((from) & 0x3f))
#define move_set_to(move, to)             ((move) |= (((to) & 0x3f) << 6))
#define move_set_promo_piece(move, piece) ((move) |= (((piece) & 0x3) << 14))

#define move_is_capture(move)    (((move) >> 12) & 1)
#define move_is_promotion(move)  (((move) >> 13) & 1)
#define move_is_quiet(move)      (!move_is_promotion((move)) && !move_is_capture((move)))
#define move_is_castle(move)     (move_is_quiet((move)) && move_promo_piece((move)))


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
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	ALL,
	PIECE_MAX,
};

enum piece_id {
	W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
	B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
	EMPTY,
	PIECE_ID_MAX
};
#define piece_color(id) ((id) >= B_PAWN)
#define piece_type(id)  ((id) - (piece_color((id)) * B_PAWN))

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
	// 0101
	KINGSIDE = WHITE_KINGSIDE | BLACK_KINGSIDE,
	// 1010
	QUEENSIDE = WHITE_QUEENSIDE | BLACK_QUEENSIDE,
	// 1111
	ANY_CASTLING = WHITE_BOTHSIDE | BLACK_BOTHSIDE
};

struct board {
	enum piece_id squares[64]; // piece_id squares
	u64 pieces[PIECE_MAX];     // piece bitboards
	u64 colors[COLOR_MAX];     // color bitboards
	enum castling castling;    // castling rights
};

#define pieces(board, piece, color) ((board)->pieces[(piece)] & (board)->colors[(color)])
#define pawns(board, color) (pieces((board), PAWN, (color)))
#define can_castle(board, color, queenside) \
	((board)->castling & (((queenside) ? WHITE_QUEENSIDE : WHITE_KINGSIDE) << 2 * (color)))

void board_init(struct board *board);
void board_put_piece(struct board *board, int square, enum piece_id id);
void board_del_piece(struct board *board, int square);
void board_move_piece(struct board *board, int from, int to);
void board_move(struct board *board, move move);
void board_undo_move(struct board *board, move move, enum piece_id captured);

#endif // TYPES_H
