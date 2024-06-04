#include "movegen.h"

#include "bitboard.h"

#include <stdlib.h>

// pre-initialized lineattacks table, used for bishops, rooks, and queens
// indexed by lineattacks[diagonal|antidiagonal|horizontal|vertcal][square]
// and gives the respective bitboard for each lineattack for the square
static u64 lineattacks[4][64];

static void init_lineattacks_table()
{
	for (int i = 0; i < 64; ++i) {
		lineattacks[DIAGONAL][i]     = diagonal(i);
		lineattacks[ANTIDIAGONAL][i] = antidiagonal(i);
		lineattacks[HORIZONTAL][i]   = rank(i);
		lineattacks[VERTICAL][i]     = file(i);
	}
}

static struct move* make_move(enum movetype type, int from, int to)
{
	struct move *move = malloc(sizeof(struct move));
	move->type = type;
	move->from = from;
	move->to   = to;
	return move;
}

static inline void add_move(struct node *list, struct move* move)
{
	list_add(list, &move->node);
}

static void generate_pawn_moves(struct board *board, struct node *list,
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
			add_move(list, make_move(type, to - up, to));
		}

		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, make_move(type, to - up - up, to));
		}
	} else if (type == PROMOTION) {
		// pawns can promote by moving or capturing while on the 7th rank
		u64 b1 = shift(rank7_pawns, up_right) & enemies;
		u64 b2 = shift(rank7_pawns, up_left) & enemies;
		u64 b3 = shift(rank7_pawns, up) & empty;

		while (b1) {
			int to = pop_lsb(b1);
			add_move(list, make_move(type, to - up_right , to));
		}

		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, make_move(type, to - up_left, to));
		}

		while (b3) {
			int to = pop_lsb(b3);
			add_move(list, make_move(type, to - up, to));
		}
	} else if (type == CAPTURE) {
		// regular captures
		u64 b1 = shift(not_rank7_pawns, up_right) & enemies;
		u64 b2 = shift(not_rank7_pawns, up_left) & enemies;

		while (b1) {
			int to = pop_lsb(b1);
			add_move(list, make_move(type, to - up_right , to));
		}
		while (b2) {
			int to = pop_lsb(b2);
			add_move(list, make_move(type, to - up_left, to));
		}
	}
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

u64 pos_ray_attacks(int square, u64 occupied, enum lineattacks type)
{
	u64 attacks = pos_ray(lineattacks[type][square], square);
	u64 blocker = lsb((attacks & occupied) | 0x8000000000000000ULL);
	return attacks ^ pos_ray(lineattacks[type][blocker], blocker);
}

u64 neg_ray_attacks(int square, u64 occupied, enum lineattacks type)
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
	switch (piece) {
	case KNIGHT: return knight_attacks_bb(square);
	case KING:   return king_attacks_bb(square);
	case BISHOP: return bishop_attacks_bb(square, occupied);
	case ROOK:   return rook_attacks_bb(square, occupied);
	case QUEEN:  return queen_attacks_bb(square, occupied);
	case PAWN:   return 0ULL; // TODO: Add assert for piece != PAWN
	}
	return 0ULL;
}

static void generate_moves(struct board *board, struct node *list,
			   enum piece piece, enum color color,
			   enum movetype type)
{
	u64 pieces = board->pieces[color][piece];
	while (pieces) {
		int from = pop_lsb(pieces);
		u64 bb   = attacks_bb(from, board->occupied[BOTH], piece);

		while (bb) {
			add_move(list, make_move(type, from, pop_lsb(bb)));
		}
	}
}
