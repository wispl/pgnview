#include "board.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// pre-initialized lineattacks table, used for bishops, rooks, and queens
static u64 lineattacks[4][64];

static u64 diagonal_mask(int square) {
   const u64 main_diagonal = 0x8040201008040201;
   int diag = 8 * (square & 7) - (square & 56);
   int north = -diag & ( diag >> 31);
   int south =  diag & (-diag >> 31);
   return (main_diagonal >> south) << north;
}

static u64 antidiagonal_mask(int square) {
   const u64 main_diagonal = 0x0102040810204080;
   int diag = 56- 8 * (square & 7) - (square & 56);
   int north = -diag & ( diag >> 31);
   int south =  diag & (-diag >> 31);
   return (main_diagonal >> south) << north;
}

static u64 rank_mask(int square)
{
	return  0xFFULL << (square & 56);
}

static u64 file_mask(int square)
{
	return 0x0101010101010101ULL << (square & 7);
}


static void init_lineattacks_table() 
{
	for (int i = 0; i < 64; ++i) {
		lineattacks[DIAGONAL][i] = diagonal_mask(i);
		lineattacks[ANTIDIAGONAL][i] = antidiagonal_mask(i);
		lineattacks[HORIZONTAL][i] = rank_mask(i);
		lineattacks[VERTICAL][i] = file_mask(i);
	}
}

//
// Bits
//

// For bitscanning
static const int index64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

static int bit_scan(u64 bb)
{
	const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
	assert (bb != 0);
	return index64[((bb ^ (bb-1)) * debruijn64) >> 58];
}

static int bit_scan_rev(u64 bb)
{
	const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
	assert (bb != 0);
	bb |= bb >> 1;
	bb |= bb >> 2;
	bb |= bb >> 4;
	bb |= bb >> 8;
	bb |= bb >> 16;
	bb |= bb >> 32;
	return index64[(bb * debruijn64) >> 58];
}

static int pop_lsb(u64 bb)
{
	int lsb = bit_scan(bb);
	bb &= bb - 1;
	return lsb;
}

static void print_bitboard(u64 bitboard)
{
	printf("bitboard: %lld\n", bitboard);
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			if (!file) {
				printf("%d ", rank + 1);
			}
			printf(" %c", get_bit(bitboard, square) ? '1' : '.');
		}
		printf("\n");
	}
	printf("\n   a b c d e f g h\n");
}

static u64 shift(u64 bb, enum direction dir)
{
	switch(dir) {
	case NORTH:      return north(bb);
	case SOUTH:      return south(bb);
	case EAST:       return east(bb);
	case NORTH_EAST: return north_east(bb);
	case SOUTH_EAST: return south_east(bb);
	case WEST:       return west(bb);
	case NORTH_WEST: return north_west(bb);
	case SOUTH_WEST: return south_west(bb);
	}
	return 0ULL;
}

//
// movelist
//

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

//
// movegen
//

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
	u64 blocker = bit_scan((attacks & occupied) | 0x8000000000000000ULL);
	return attacks ^ pos_ray(lineattacks[type][blocker], blocker);
}

u64 neg_ray_attacks(int square, u64 occupied, enum lineattacks type)
{
	u64 attacks = neg_ray(lineattacks[type][square], square);
	u64 blocker = bit_scan_rev((attacks & occupied) | 1);
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

