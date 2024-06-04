#include "movegen.h"

#include "bitboard.h"

#include <assert.h>
#include <stdlib.h>

// pre-initialized lineattacks table, used for bishops, rooks, and queens
// indexed by lineattacks[diagonal|antidiagonal|horizontal|vertcal][square]
// and gives the respective bitboard for each lineattack for the square
static u64 lineattacks[4][64];

void init_lineattacks_table()
{
	for (int i = 0; i < 64; ++i) {
		lineattacks[DIAGONAL][i]     = diagonal(i);
		lineattacks[ANTIDIAGONAL][i] = antidiagonal(i);
		lineattacks[HORIZONTAL][i]   = rank(i);
		lineattacks[VERTICAL][i]     = file(i);
	}
}

static void add_move(struct movelist *list, enum movetype type, int from, int to)
{
	int index = list->len;
	list->moves[index].type = type;
	list->moves[index].from = from;
	list->moves[index].to   = to;
	++list->len;
}

static u64 knight_attacks_bb(int square)
{
	u64 bitboard = 0ULL;
	set_bit(bitboard, square);

	u64 west, east, attacks;
	east     = east(bitboard);
	west     = west(bitboard);
	attacks  = (east|west) << 16;
	attacks |= (east|west) >> 16;
	east     = east(east);
	west     = west(west);
	attacks |= (east|west) <<  8;
	attacks |= (east|west) >>  8;
	return attacks;
}

static u64 king_attacks_bb(int square)
{
	u64 bitboard = 0ULL;
	set_bit(bitboard, square);

	u64 attacks = east(bitboard) | west(bitboard);
	bitboard    |= attacks;
	attacks     |= north(bitboard) | south(bitboard);
	return attacks;
}

// classical method to determine squares for queens, rooks, and bishops
static u64 pos_ray_attacks(int square, u64 occupied, enum lineattacks type)
{
	u64 attacks = pos_ray(lineattacks[type][square], square);
	u64 blocker = lsb((attacks & occupied) | 0x8000000000000000ULL);
	return attacks ^ pos_ray(lineattacks[type][blocker], blocker);
}

static u64 neg_ray_attacks(int square, u64 occupied, enum lineattacks type)
{
	u64 attacks = neg_ray(lineattacks[type][square], square);
	u64 blocker = msb((attacks & occupied) | 1);
	return attacks ^ neg_ray(lineattacks[type][blocker], blocker);
}

static u64 bishop_attacks_bb(int square, u64 occupied)
{
	return pos_ray_attacks(square, occupied, DIAGONAL)
	     | neg_ray_attacks(square, occupied, DIAGONAL)
	     | pos_ray_attacks(square, occupied, ANTIDIAGONAL)
	     | neg_ray_attacks(square, occupied, ANTIDIAGONAL);
}

static u64 rook_attacks_bb(int square, u64 occupied)
{
	return pos_ray_attacks(square, occupied, HORIZONTAL)
	     | neg_ray_attacks(square, occupied, HORIZONTAL)
	     | pos_ray_attacks(square, occupied, VERTICAL)
	     | neg_ray_attacks(square, occupied, VERTICAL);
}

static u64 queen_attacks_bb(int square, u64 occupied)
{
	return bishop_attacks_bb(square, occupied)
	     | rook_attacks_bb(square, occupied);
}

static u64 attacks_bb(int square, u64 occupied, enum piece piece)
{
	assert(piece != PAWN);
	switch (piece) {
	case KNIGHT: return knight_attacks_bb(square);
	case KING:   return king_attacks_bb(square);
	case BISHOP: return bishop_attacks_bb(square, occupied);
	case ROOK:   return rook_attacks_bb(square, occupied);
	case QUEEN:  return queen_attacks_bb(square, occupied);
	case PAWN:   return 0ULL;
	}
	return 0ULL;
}

// TODO: handle en passant
static void generate_pawn_moves(struct board *board, struct movelist *list,
				enum color color, enum movetype type)
{
	u64 empty    = ~board->occupied[BOTH];
	u64 enemies  =  board->occupied[(color == WHITE) ? BLACK : WHITE];

	enum direction up    	= (color == WHITE) ? NORTH      : SOUTH;
	enum direction up_right = (color == WHITE) ? NORTH_EAST : SOUTH_WEST;
	enum direction up_left  = (color == WHITE) ? NORTH_WEST : SOUTH_EAST;

	// pawns which can promote
	u64 rank7 = (color == WHITE) ? rank_7 : rank_2;
	u64 rank7_pawns = board->pieces[color][PAWN] & rank7;
	// pawns which can't promote
	u64 not_rank7_pawns = board->pieces[color][PAWN] & ~rank7;

	// pawns which can move two squares
	u64 rank2 = (color == WHITE) ? rank_2 : rank_7;
	u64 rank2_pawns = board->pieces[color][PAWN] & rank2;

	if (type == QUIET) {
		u64 b1 = shift(not_rank7_pawns, up) & empty;
		u64 b2 = shift(shift(rank2_pawns, up), up) & empty;

		while (b1) {
			int to = pop_lsb(b1);
			add_move(list, type, to - up, to);
		}

		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, type, to - up - up, to);
		}
	} else if (type == PROMOTION) {
		// pawns can promote by moving or capturing while on the 7th rank
		u64 b1 = shift(rank7_pawns, up_right) & enemies;
		u64 b2 = shift(rank7_pawns, up_left) & enemies;
		u64 b3 = shift(rank7_pawns, up) & empty;

		while (b1) {
			int to = pop_lsb(b1);
			add_move(list, type, to - up_right , to);
		}

		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, type, to - up_left, to);
		}

		while (b3) {
			int to = pop_lsb(b3);
			add_move(list, type, to - up, to);
		}
	} else if (type == CAPTURE) {
		// regular captures
		u64 b1 = shift(not_rank7_pawns, up_right) & enemies;
		u64 b2 = shift(not_rank7_pawns, up_left) & enemies;

		while (b1) {
			int to = pop_lsb(b1);
			add_move(list, type, to - up_right , to);
		}
		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, type, to - up_left, to);
		}
	}
}

void init_board(struct board *board)
{
	board->pieces[WHITE][KING]   = square_bb(e1);
	board->pieces[WHITE][QUEEN]  = square_bb(d1);
	board->pieces[WHITE][ROOK]   = square_bb(a1) | square_bb(h1);
	board->pieces[WHITE][BISHOP] = square_bb(c1) | square_bb(f1);
	board->pieces[WHITE][KNIGHT] = square_bb(b1) | square_bb(g1);
	board->pieces[WHITE][PAWN]   = rank_2;

	board->pieces[BLACK][KING]   = square_bb(e8);
	board->pieces[BLACK][QUEEN]  = square_bb(d8);
	board->pieces[BLACK][ROOK]   = square_bb(a8) | square_bb(h8);
	board->pieces[BLACK][BISHOP] = square_bb(c8) | square_bb(f8);
	board->pieces[BLACK][KNIGHT] = square_bb(b8) | square_bb(g8);
	board->pieces[BLACK][PAWN]   = rank_7;

	board->occupied[WHITE]       = rank_1 | rank_2;
	board->occupied[BLACK]       = rank_7 | rank_8;
	board->occupied[BOTH]        = board->occupied[WHITE] | board->occupied[BLACK];
}

void generate_moves(struct board *board, struct movelist *list, enum piece piece,
		    enum color color, enum movetype type)
{
	assert(type != PROMOTION && piece != PAWN);
	if (piece == PAWN) {
		generate_pawn_moves(board, list, color, type);
		return;
	}

	u64 pieces  =  board->pieces[color][piece];
	u64 empty   = ~board->occupied[BOTH];
	u64 enemies =  board->occupied[(color == WHITE) ? BLACK : WHITE];
	while (pieces) {
		int from = pop_lsb(pieces);
		u64 bb   = attacks_bb(from, board->occupied[BOTH], piece);
		if (type == CAPTURE) {
			bb &= enemies;
		} else if (type == QUIET) {
			bb &= empty;
		}

		while (bb) {
			add_move(list, type, from, pop_lsb(bb));
		}
	}
}

void init_board(struct board *board)
{
	board->pieces[WHITE][KING]   = square_bb(e1);
	board->pieces[WHITE][QUEEN]  = square_bb(d1);
	board->pieces[WHITE][ROOK]   = square_bb(a1) | square_bb(h1);
	board->pieces[WHITE][BISHOP] = square_bb(c1) | square_bb(f1);
	board->pieces[WHITE][KNIGHT] = square_bb(b1) | square_bb(g1);
	board->pieces[WHITE][PAWN]   = rank_2;

	board->pieces[BLACK][KING]   = square_bb(e8);
	board->pieces[BLACK][QUEEN]  = square_bb(d8);
	board->pieces[BLACK][ROOK]   = square_bb(a8) | square_bb(h8);
	board->pieces[BLACK][BISHOP] = square_bb(c8) | square_bb(f8);
	board->pieces[BLACK][KNIGHT] = square_bb(b8) | square_bb(g8);
	board->pieces[BLACK][PAWN]   = rank_7;

	board->occupied[WHITE]       = rank_1 | rank_2;
	board->occupied[BLACK]       = rank_7 | rank_8;
	board->occupied[BOTH]        = board->occupied[WHITE] | board->occupied[BLACK];
}

void movelist_clear(struct movelist *list)
{
	list->len = 0;
}
