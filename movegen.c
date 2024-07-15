#include "movegen.h"

#include "types.h"

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
static bool initialized = false;

bool attacks_table_initilized()
{
	return initialized;
}

void init_lineattacks_table()
{
	initialized = true;
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

static move* all_promotions(move *moves, int from, int to, bool is_capture)
{
	for (int i = 0; i < 4; ++i)
		*moves++ = make_promotion(from, to, is_capture, i);
	return moves;
}

// TODO: handle en passant
static move* generate_pawn_moves(struct board *board, move *moves, struct movegenc *conf)
{
	enum gentype movetype = conf->type;
	enum color color = conf->color;
	u64 target = conf->target;

	u64 empty   = ~board->pieces[ALL] & target;
	u64 enemies =  board->colors[flip_color(color)] & target;

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
		
		while (b1) {
			int to = pop_lsb(&b1);
			*moves++ = make_quiet(to - up, to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			*moves++ = make_quiet(to - up - up, to);
		}
	} else if (movetype == PROMO_CAPTURE) {
		u64 b1 = shift(rank7_pawns, up_right) & enemies;
		u64 b2 = shift(rank7_pawns, up_left) & enemies;

		while (b1) {
			int to = pop_lsb(&b1);
			moves = all_promotions(moves, to - up_right, to, true);
		}

		while (b2) {
			int to = pop_lsb(&b2);
			moves = all_promotions(moves, to - up_left, to, true);
		}
	} else if (movetype == PROMOTION) {
		u64 b1 = shift(rank7_pawns, up) & empty;

		while (b1) {
			int to = pop_lsb(&b1);
			moves = all_promotions(moves, to - up, to, false);
		}
	} else if (movetype == CAPTURE) {
		u64 b1 = shift(not_rank7_pawns, up_right) & enemies;
		u64 b2 = shift(not_rank7_pawns, up_left) & enemies;

		while (b1) {
			int to = pop_lsb(&b1);
			*moves++ = make_capture(to - up_right, to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			*moves++ = make_capture(to - up_left, to);
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
static move* generate_castle_moves(struct board *board, move *moves, struct movegenc *conf)
{
	u64 king = (conf->color == WHITE) ? e1 : e8;
	for (int i = 0; i < 2; ++i) {
		if (can_castle(board, conf->color, i)) {
			u64 bb = h_ray(king, board->pieces[ALL], i) & conf->target;
			if (bb)
				*moves++ = make_castle(king, pop_lsb(&bb));
		}
	}
	return moves;
}

move* generate_moves(struct board *board, move *moves, struct movegenc *conf)
{
	assert(initialized == true);

	if (conf->piece == PAWN)
		return generate_pawn_moves(board, moves, conf);

	if (conf->type == CASTLE)
		return generate_castle_moves(board, moves, conf);

	assert(conf->type != PROMOTION && conf->type != PROMO_CAPTURE);

	u64 pieces   =  pieces(board, conf->piece, conf->color);
	u64 occupied =  board->pieces[ALL];
	u64 empty    = ~occupied;
	u64 enemies  =  board->colors[flip_color(conf->color)];
	bool is_capture = (conf->type == CAPTURE);

	while (pieces) {
		int from = pop_lsb(&pieces);
		u64 bb   = attacks_bb(from, occupied, conf->piece);
		bb &= ((is_capture) ? enemies : empty) & conf->target;

		while (bb)
			*moves++ = make_move(from, pop_lsb(&bb), is_capture, false, 0);
	}
	return moves;
}
