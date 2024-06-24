#include "movegen.h"

#include "bitboard.h"
#include "board.h"
#include "move.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>

enum lineattacks {
	DIAGONAL,	
	ANTIDIAGONAL,
	HORIZONTAL,
	VERTICAL,
};

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

static struct move make_move(enum movetype movetype, int from, int to)
{
	return (struct move) {
		.movetype = movetype,
		.from = from,
		.to = to
	};
}

static u64 knight_attacks_bb(int square)
{
	u64 target = square_bb(square);
	u64 west, east, attacks;
	east     = east(target);
	west     = west(target);
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
	u64 target = square_bb(square);
	u64 attacks = east(target) | west(target);
	target     |= attacks;
	attacks    |= north(target) | south(target);
	return attacks;
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
	assert(piece != PAWN && piece != ALL);
	switch (piece) {
	case KNIGHT: return knight_attacks_bb(square);
	case KING:   return king_attacks_bb(square);
	case BISHOP: return bishop_attacks_bb(square, occupied);
	case ROOK:   return rook_attacks_bb(square, occupied);
	case QUEEN:  return queen_attacks_bb(square, occupied);
	default:     return 0ULL;
	}
	return 0ULL;
}

// TODO: handle en passant
static struct move* generate_pawn_moves(struct board *board, struct move *moves,
		                        struct movegenc *conf)
{
	enum movetype movetype = conf->movetype;
	enum color color = conf->color;
	u64 target = conf->target;

	u64 empty   = ~board->pieces[ALL];
	u64 enemies =  board->colors[flip_color(color)];

	enum direction up    	= (color == WHITE) ? NORTH      : SOUTH;
	enum direction up_right = (color == WHITE) ? NORTH_EAST : SOUTH_WEST;
	enum direction up_left  = (color == WHITE) ? NORTH_WEST : SOUTH_EAST;

	// pawns on rank 7 can promote by moving or capturing onto rank 8
	u64 rank7           = (color == WHITE) ? rank_7 : rank_2;
	u64 rank7_pawns     = pawns(board, color) & rank7;
	u64 not_rank7_pawns = pawns(board, color) & ~rank7;

	// pawns which can move two squares
	u64 rank2       = (color == WHITE) ? rank_2 : rank_7;
	u64 rank2_pawns = pawns(board, color) & rank2;

	if (movetype == QUIET) {
		u64 b1 = shift(not_rank7_pawns, up) & empty;
		u64 b2 = shift(shift(rank2_pawns, up), up) & empty;
		b1 &= target;
		b2 &= target;
		
		while (b1) {
			int to = pop_lsb(&b1);
			*moves++ = make_move(QUIET, to - up, to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			*moves++ = make_move(QUIET, to - up - up, to);
		}
	} else if (movetype == PROMOTION) {
		u64 b1 = shift(rank7_pawns, up_right) & enemies;
		u64 b2 = shift(rank7_pawns, up_left) & enemies;
		u64 b3 = shift(rank7_pawns, up) & empty;
		b1 &= target;
		b2 &= target;
		b3 &= target;

		while (b1) {
			int to = pop_lsb(&b1);
			*moves++ = make_move(PROMOTION, to - up_right, to);
		}

		while (b2) {
			int to = pop_lsb(&b2);
			*moves++ = make_move(PROMOTION, to - up_left, to);
		}

		while (b3) {
			int to = pop_lsb(&b3);
			*moves++ = make_move(PROMOTION, to - up, to);
		}
	} else if (movetype == CAPTURE) {
		// regular captures
		u64 b1 = shift(not_rank7_pawns, up_right) & enemies;
		u64 b2 = shift(not_rank7_pawns, up_left) & enemies;
		b1 &= target;
		b2 &= target;

		while (b1) {
			int to = pop_lsb(&b1);
			*moves++ = make_move(CAPTURE, to - up_right, to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			*moves++ = make_move(CAPTURE, to - up_left, to);
		}
	}
	return moves;
}

static u64 h_ray(int square, u64 occupied, bool is_negative)
{
	return (is_negative) ? neg_ray_attacks(square, occupied, HORIZONTAL)
	                     : pos_ray_attacks(square, occupied, HORIZONTAL);
}

// TODO: check for attacks on squares between rook and king
static struct move* generate_castle_moves(struct board *board, struct move *moves,
		                          struct movegenc *conf)
{
	u64 king = (conf->color == WHITE) ? e1 : e8;
	for (int i = 0; i < 2; ++i) {
		if (can_castle(board, conf->color, i)) {
			u64 bb = h_ray(king, board->pieces[ALL], i) & conf->target;
			if (bb) {
				*moves++ = make_move(CASTLE, king, pop_lsb(&bb));
				// TODO: check if bb = 0, it should be
			}
		}
	}
	return moves;
}

struct move* generate_moves(struct board *board, struct move *moves, struct movegenc *conf)
{
	if (conf->piece == PAWN) {
		return generate_pawn_moves(board, moves, conf);
	}

	if (conf->movetype == CASTLE) {
		return generate_castle_moves(board, moves, conf);
	}

	assert(conf->movetype != PROMOTION);

	u64 pieces   =  pieces(board, conf->piece, conf->color);
	u64 occupied =  board->pieces[ALL];
	u64 empty    = ~occupied;
	u64 enemies  =  board->colors[flip_color(conf->color)];
	while (pieces) {
		int from = pop_lsb(&pieces);
		u64 bb   = attacks_bb(from, occupied, conf->piece);
		bb &= (conf->movetype == CAPTURE) ? enemies : empty;
		bb &= conf->target;

		while (bb) {
			*moves++ = make_move(conf->movetype, from, pop_lsb(&bb));
		}
	}
	return moves;
}
