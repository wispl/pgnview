#include "types.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static inline enum piece_id make_piece(enum piece piece, enum color color)
{
	return piece + (color * B_PAWN);
}

void board_init(struct board *board)
{
	board->colors[WHITE]  = rank_1 | rank_2;
	board->colors[BLACK]  = rank_7 | rank_8;

	board->pieces[KING]   = square_bb(e1) | square_bb(e8);
	board->pieces[QUEEN]  = square_bb(d1) | square_bb(d8);
	board->pieces[ROOK]   = square_bb(a1) | square_bb(h1)
			      | square_bb(a8) | square_bb(h8);
	board->pieces[BISHOP] = square_bb(c1) | square_bb(f1)
	                      | square_bb(c8) | square_bb(f8);
	board->pieces[KNIGHT] = square_bb(b1) | square_bb(g1)
			      | square_bb(b8) | square_bb(g8);
	board->pieces[PAWN]   = rank_2 | rank_7;
	board->pieces[ALL]    = rank_1 |rank_2 | rank_7 | rank_8;
	int squares[64] = {
		W_ROOK, W_KNIGHT, W_BISHOP, W_QUEEN, W_KING, W_BISHOP, W_KNIGHT, W_ROOK,
		W_PAWN, W_PAWN,   W_PAWN,   W_PAWN,  W_PAWN, W_PAWN,   W_PAWN,   W_PAWN,
		EMPTY,  EMPTY,    EMPTY,    EMPTY,   EMPTY,  EMPTY,    EMPTY,    EMPTY,
		EMPTY,  EMPTY,    EMPTY,    EMPTY,   EMPTY,  EMPTY,    EMPTY,    EMPTY,
		EMPTY,  EMPTY,    EMPTY,    EMPTY,   EMPTY,  EMPTY,    EMPTY,    EMPTY,
		EMPTY,  EMPTY,    EMPTY,    EMPTY,   EMPTY,  EMPTY,    EMPTY,    EMPTY,
		B_PAWN, B_PAWN,   B_PAWN,   B_PAWN,  B_PAWN, B_PAWN,   B_PAWN,   B_PAWN,
		B_ROOK, B_KNIGHT, B_BISHOP, B_QUEEN, B_KING, B_BISHOP, B_KNIGHT, B_ROOK
	};
	memcpy(board->squares, squares, 64 * sizeof(enum piece));
	board->castling = ANY_CASTLING;
}

void board_put_piece(struct board *board, int square, enum piece_id id)
{
	u64 bb = square_bb(square);

	board->pieces[ALL]             |= bb;
	board->pieces[piece_type(id)]  |= bb;
	board->colors[piece_color(id)] |= bb;

	board->squares[square] = id;
}

void board_del_piece(struct board *board, int square)
{
	enum piece_id id = board->squares[square];
	u64 bb = square_bb(square);

	board->pieces[ALL]             ^= bb;
	board->pieces[piece_type(id)]  ^= bb;
	board->colors[piece_color(id)] ^= bb;

	board->squares[square] = EMPTY;
}

void board_move_piece(struct board *board, int from, int to)
{
	enum piece_id id = board->squares[from];
	u64 from_to = square_bb(from) | square_bb(to);

	board->pieces[ALL]             ^= from_to;
	board->pieces[piece_type(id)]  ^= from_to;
	board->colors[piece_color(id)] ^= from_to;

	board->squares[from] = EMPTY;
	board->squares[to]   = id;
}

void board_move(struct board *board, move move)
{
	int from = move_from(move);
	int to   = move_to(move);
	enum color color = piece_color(board->squares[from]);

	// update castling rights
	// king moved
	board->castling &= ~(WHITE_BOTHSIDE * (from == e1));
	board->castling &= ~(BLACK_BOTHSIDE * (from == e8));
	// rook moved or got captured
	board->castling &= ~(WHITE_KINGSIDE * (from == h1 || to == h1));
	board->castling &= ~(WHITE_QUEENSIDE * (from == a1 || to == a1));
	board->castling &= ~(BLACK_KINGSIDE * (from == h8 || to == h8));
	board->castling &= ~(BLACK_QUEENSIDE * (from == a8 || to == a8));

	if (move_is_castle(move)) {
		bool kingside = to > from;
		int king = from + ((kingside) ?  2 : -2);
		int rook = to   + ((kingside) ? -2 :  3);

		board_move_piece(board, from, king);
		board_move_piece(board, to,   rook);
		return;
	}

	if (move_is_capture(move))
		board_del_piece(board, to);

	board_move_piece(board, from, to);

	if (move_is_promotion(move)) {
		enum piece piece = move_promo_piece(move) + 1;
		board_del_piece(board, to);
		board_put_piece(board, to, make_piece(piece, color));
	}
}

// TODO: regain castling rights?
void board_undo_move(struct board *board, move move, enum piece_id captured)
{
	int from = move_from(move);
	int to   = move_to(move);
	enum color color = piece_color(board->squares[to]);

	if (move_is_castle(move)) {
		bool kingside = to > from;
		int king = from + ((kingside) ?  2 : -2);
		int rook = to   + ((kingside) ? -2 :  3);

		board_move_piece(board, king, from);
		board_move_piece(board, rook, to);
		return;
	}

	if (move_is_promotion(move)) {
		board_del_piece(board, to);
		board_put_piece(board, to, make_piece(PAWN, color));
	}

	board_move_piece(board, to, from);

	if (move_is_capture(move))
		board_put_piece(board, to, captured);
}
