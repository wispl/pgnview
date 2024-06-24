#include "board.h"

#include "bitboard.h"
#include "move.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char piece_str[PIECE_ID_MAX] = {
	[W_KING]   = 'K',
	[W_QUEEN]  = 'Q',
	[W_ROOK]   = 'R',
	[W_BISHOP] = 'B',
	[W_KNIGHT] = 'N',
	[W_PAWN]   = 'P',
	[B_KING]   = 'k',
	[B_QUEEN]  = 'q',
	[B_ROOK]   = 'r',
	[B_BISHOP] = 'b',
	[B_KNIGHT] = 'n',
	[B_PAWN]   = 'p',
	[EMPTY]    = '.',
};

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
	if (move_is_castle(move)) {
		bool kingside = to > from;
		int king = from + ((kingside) ?  2 : -2);
		int rook = to   + ((kingside) ? -2 :  3);

		board_move_piece(board, from, king);
		board_move_piece(board, to,   rook);
	} else if (move_is_quiet(move)) {
		board_move_piece(board, from, to);

		enum color color = piece_color(board->squares[from]);
		enum piece piece = piece_type(board->squares[from]);

		if (piece == KING) {
			board->castling &= ~(WHITE_BOTHSIDE << (color * 2));
		} else if (piece == ROOK) {
			enum castling mask = (from < to) ? QUEENSIDE : KINGSIDE;
			board->castling &= mask & color;
		}
	} else if (move_is_capture(move)) {
		board_del_piece(board, to);
		board_move_piece(board, from, to);
	}
}

void board_undo_move(struct board *board, move move, enum piece_id captured)
{
	int from = move_from(move);
	int to   = move_to(move);
	if (move_is_castle(move)) {
		bool kingside = to > from;
		int king = from + ((kingside) ?  2 : -2);
		int rook = to   + ((kingside) ? -2 :  3);

		board_move_piece(board, king, from);
		board_move_piece(board, rook, to);
	} else if (move_is_quiet(move)) {
		board_move_piece(board, to, from);
	} else if (move_is_capture(move)) {
		board_move_piece(board, to, from);
		board_put_piece(board, to, captured);
	}
}

void board_print(struct board *board)
{
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			if (!file) {
				printf("%d ", rank + 1);
			}
			printf(" %c", piece_str[board->squares[square]]);
		}
		printf("\n");
	}
	printf("\n   a b c d e f g h\n");
}
