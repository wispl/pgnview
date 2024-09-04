#ifndef CHESS_H
#define CHESS_H

#include <stdbool.h>
#include <stdint.h>

// This file contains chess related methods and structure. The three main structs
// are bitboard, board, and move.
//
// Bitboards are a compact representation of the chess board using 64 bits.
// They encode information only about occupancy.
//
// Moves encode various information about a single ply (half turn).
//
// A board represent information and state about a position.
//
// movegenc serves as an auxillary struct for generating moves for a given
// position.
//
// Currently, the implementation here generates pseudo-legal moves,
// which means the moves generated do NOT ensure the king is in check, as
// opposed to legal moves which ensure the king is not in check and that the
// move is legal.

// Module bitboard.c

typedef unsigned long long u64;

// Little endian ranked-file ordered square mapping.
// Note the mapping determines implementations of bit manipulation functions,
// having this rank based order allows for easy shifting of pawns.
enum squares {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8,
	SQUARES_NONE
};

// Creates a bitboard from a square.
#define square_bb(square) (1ULL << (square))

void print_bitboard(u64 bitboard);

// TODO: these should probably be uppercase
// File and rank constants for bitboards
#define file_a 0x0101010101010101ULL
#define file_b (file_a << 1)
#define file_c (file_a << 2)
#define file_d (file_a << 3)
#define file_e (file_a << 4)
#define file_f (file_a << 5)
#define file_g (file_a << 6)
#define file_h (file_a << 7)

#define rank_1 0xFFULL
#define rank_2 (rank_1 << (8 * 1))
#define rank_3 (rank_1 << (8 * 2))
#define rank_4 (rank_1 << (8 * 3))
#define rank_5 (rank_1 << (8 * 4))
#define rank_6 (rank_1 << (8 * 5))
#define rank_7 (rank_1 << (8 * 6))
#define rank_8 (rank_1 << (8 * 7))

// a1-h8 diagonal
#define MAIN_DIAGONAL      0x8040201008040201ULL
// a8-h1 diagonal
#define MAIN_ANTIDIAGONAL  0x0102040810204080ULL

// Gets the rank, file, diagonal and antidiagonal associated with the square.
#define rank(square) (0xFFULL << ((square) & 56))

#define file(square) (0x0101010101010101ULL << ((square) & 7))

static inline u64 diagonal(int square)
{
	int diag = (square & 7) - (square >> 3);
	return diag >= 0 ? MAIN_DIAGONAL >> diag * 8 : MAIN_DIAGONAL << -diag * 8;
}

static inline u64 antidiagonal(int square)
{
	int diag = 7 - (square & 7) - (square >> 3);
	return diag >= 0 ? MAIN_ANTIDIAGONAL >> diag * 8 : MAIN_ANTIDIAGONAL << -diag * 8;
}

// Gets positive or negative portion of rank, file, diagonal, or antidiagonal.
#define pos_ray(line, square) ((line) & (-2ULL << (square)))
#define neg_ray(line, square) ((line) & ((1ULL << (square)) - 1))

enum direction {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,
    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
};

// Shift function and macros
u64 shift(u64 bb, enum direction dir);
#define north(b)      ((b << 8))
#define south(b)      ((b >> 8))
#define east(b)       ((b << 1) & ~file_a)
#define north_east(b) ((b << 9) & ~file_a)
#define south_east(b) ((b >> 7) & ~file_a)
#define west(b)       ((b >> 1) & ~file_h)
#define north_west(b) ((b << 7) & ~file_h)
#define south_west(b) ((b >> 9) & ~file_h)

// Bit scan and bit manipulation functions
static inline int lsb(u64 bb)
{
#if   defined(__GNUC__)
	return (int) __builtin_ctzll(bb);
#elif defined(_MSC_VER)
	unsigned long i;
	_BitScanForward64(&i, bb);
	return i;
#else
	#error "Compiler not supported"
#endif
}

static inline int msb(u64 bb)
{
#if   defined(__GNUC__)
	return (int) 63 ^ __builtin_clzll(bb);
#elif defined(_MSC_VER)
	unsigned long i;
	_BitScanReverse64(&i, bb);
	return i;
#else
	#error "Compiler not supported"
#endif
}

static inline int pop_lsb(u64 *bb)
{
	int i = lsb(*bb);
	*bb &= *bb - 1;
	return i;
}

// 6 bits for from square
// 6 bits for to square
// 1 bit for capture flag
// 1 bit for promotion flag
// 2 bits for special flags (promo piece or castling)
//
// for special flags
// if promotion flag is set:
// 00 = knight
// 01 = bishop
// 10 = rook
// 11 = queen
//
// if capture flag is set but not promotion flag:
// not 0 = en passant
//
// otherwise:
// not 0 = castle
typedef uint16_t move;

#define make_move(from, to, capture, promo, special) \
	((from) + ((to) << 6) + ((capture) << 12) + ((promo) << 13) + ((special) << 14))
#define make_quiet(from, to)	(make_move((from), (to), false, false, 0))
#define make_capture(from, to)	(make_move((from), (to), true, false, 0))
#define make_castle(from, to)	(make_move((from), (to), false, false, true))
#define make_promotion(from, to, is_capture, piece) \
	(make_move((from), (to), (is_capture), true, (piece)))
#define make_enpassant(from, to) \
	(make_move((from), (to), true, false, true))

#define move_from(move)		((move) & 0x3f)
#define move_to(move)		(((move) >> 6) & 0x3f)
#define move_promo_piece(move)	(((move) >> 14) & 0x3)

#define move_set_from(move, from)         ((move) |= ((from) & 0x3f))
#define move_set_to(move, to)             ((move) |= (((to) & 0x3f) << 6))
#define move_set_promo_piece(move, piece) ((move) |= (((piece) & 0x3) << 14))

#define move_is_capture(move)    (((move) >> 12) & 1)
#define move_is_promotion(move)  (((move) >> 13) & 1)
#define move_is_quiet(move)      (!move_is_promotion((move)) && !move_is_capture((move)))
#define move_is_castle(move)     (move_is_quiet((move)) && move_promo_piece((move)))
#define move_is_enpassant(move)  \
	(move_is_capture((move)) && !move_is_promotion((move)) && move_promo_piece((move)))

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

// Castling rights using bits
// (WHITE_KINGSIDE)(WHITE_QUEENSIDE)(BLACK_KINGSIDE)(BLACK_QUEENSIDE)
enum castling {
	NO_CASTLING,
	WHITE_KINGSIDE  = 1,
	WHITE_QUEENSIDE = WHITE_KINGSIDE << 1,
	WHITE_BOTHSIDE  = WHITE_KINGSIDE | WHITE_QUEENSIDE,
	BLACK_KINGSIDE  = WHITE_KINGSIDE << 2,
	BLACK_QUEENSIDE = WHITE_KINGSIDE << 3,
	BLACK_BOTHSIDE  = BLACK_KINGSIDE | BLACK_QUEENSIDE,
	KINGSIDE = WHITE_KINGSIDE | BLACK_KINGSIDE,
	QUEENSIDE = WHITE_QUEENSIDE | BLACK_QUEENSIDE,
	ANY_CASTLING = WHITE_BOTHSIDE | BLACK_BOTHSIDE
};

// Module board.c

struct board {
	enum piece_id squares[64]; // piece_id squares
	u64 pieces[PIECE_MAX];     // piece bitboards
	u64 colors[COLOR_MAX];     // color bitboards
	enum castling castling;    // castling rights
	int ep_square;
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

enum gentype {
	QUIET,
	CASTLE,
	CAPTURE,
	PROMOTION,
	PROMO_CAPTURE
};

// Parameters to passed into generate_moves
struct movegenc {
	enum gentype type;	// type of move to generate
	enum piece piece;	// piece to generate moves for
	enum color color;	// color to generate moves for
	u64 target;		// targets bitboard
};


// Module movegen.c

bool attacks_table_initilized();
void init_lineattacks_table();
move* generate_moves(struct board *board, move *moves, struct movegenc *conf);

#endif // CHESS_H
