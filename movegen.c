#include "movegen.h"

#include "bitboard.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// pre-initialized lineattacks table, used for bishops, rooks, and queens
// indexed by lineattacks[diagonal|antidiagonal|horizontal|vertcal][square]
// and gives the respective bitboard for each lineattack for the square
static u64 lineattacks[4][64];

static const char* piece_str[PIECE_ID_MAX] = {
	[W_KING]   = "WK",
	[W_QUEEN]  = "WQ",
	[W_ROOK]   = "WR",
	[W_BISHOP] = "WB",
	[W_KNIGHT] = "WN",
	[W_PAWN]   = "WP",
	[B_KING]   = "BK",
	[B_QUEEN]  = "BQ",
	[B_ROOK]   = "BR",
	[B_BISHOP] = "BB",
	[B_KNIGHT] = "BN",
	[B_PAWN]   = "BP",
	[EMPTY]    = "..",
};

void init_lineattacks_table()
{
	for (int i = 0; i < 64; ++i) {
		lineattacks[DIAGONAL][i]     = diagonal(i);
		lineattacks[ANTIDIAGONAL][i] = antidiagonal(i);
		lineattacks[HORIZONTAL][i]   = rank(i);
		lineattacks[VERTICAL][i]     = file(i);
	}
}

static void add_move(struct movebuf *buf, enum movetype movetype, int from, int to)
{
	int index = buf->len;
	buf->moves[index].movetype = movetype;
	buf->moves[index].from = from;
	buf->moves[index].to   = to;
	++buf->len;
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
static void generate_pawn_moves(struct board *board, struct movebuf *buf,
				enum color color, enum movetype movetype)
{
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
		
		while (b1) {
			int to = pop_lsb(&b1);
			add_move(buf, QUIET, to - up, to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			add_move(buf, QUIET, to - up - up, to);
		}
	} else if (movetype == PROMOTION) {
		u64 b1 = shift(rank7_pawns, up_right) & enemies;
		u64 b2 = shift(rank7_pawns, up_left) & enemies;
		u64 b3 = shift(rank7_pawns, up) & empty;

		while (b1) {
			int to = pop_lsb(&b1);
			add_move(buf, PROMOTION, to - up_right , to);
		}

		while (b2) {
			int to = pop_lsb(&b2);
			add_move(buf, PROMOTION, to - up_left, to);
		}

		while (b3) {
			int to = pop_lsb(&b3);
			add_move(buf, PROMOTION, to - up, to);
		}
	} else if (movetype == CAPTURE) {
		// regular captures
		u64 b1 = shift(not_rank7_pawns, up_right) & enemies;
		u64 b2 = shift(not_rank7_pawns, up_left) & enemies;

		while (b1) {
			int to = pop_lsb(&b1);
			add_move(buf, CAPTURE, to - up_right , to);
		}
		while (b2) {
			int to = pop_lsb(&b2);
			add_move(buf, CAPTURE, to - up_left, to);
		}
	}
}

void generate_moves(struct board *board, struct movebuf *buf, struct movegenc *conf)
{
	// clear the buffer
	buf->len = 0;
	if (conf->piece == PAWN) {
		generate_pawn_moves(board, buf, conf->color, conf->movetype);
		return;
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
		while (bb) {
			add_move(buf, conf->movetype, from, pop_lsb(&bb));
		}
	}
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
}

void board_add(struct board *board, int square, enum piece_id id)
{
	u64 bb = square_bb(square);

	board->pieces[ALL]             |= bb;
	board->pieces[piece_type(id)]  |= bb;
	board->colors[piece_color(id)] |= bb;

	board->squares[square] = id;
}

void board_remove(struct board *board, int square)
{
	enum piece_id id = board->squares[square];
	u64 bb = square_bb(square);

	board->pieces[ALL]             ^= bb;
	board->pieces[piece_type(id)]  ^= bb;
	board->colors[piece_color(id)] ^= bb;

	board->squares[square] = EMPTY;
}

void board_move(struct board *board, struct move *move)
{
	enum piece_id id = board->squares[move->from];
	u64 from_to = square_bb(move->from) | square_bb(move->to);

	board->pieces[ALL]             ^= from_to;
	board->pieces[piece_type(id)]  ^= from_to;
	board->colors[piece_color(id)] ^= from_to;

	board->squares[move->from] = EMPTY;
	board->squares[move->to]   = id;
}

void board_print(struct board *board)
{
	for (int rank = 7; rank >= 0; --rank) {
		for (int file = 0; file < 8; ++file) {
			int square = rank * 8 + file;
			if (!file) {
				printf("%d ", rank + 1);
			}
			printf(" %s", piece_str[board->squares[square]]);
		}
		printf("\n");
	}
	printf("\n   a  b  c  d  e  f  g  h\n");
}
