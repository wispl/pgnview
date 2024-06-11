#ifndef BITBOARD_H
#define BITBOARD_H

typedef unsigned long      u32;
typedef unsigned long long u64;

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

// square to bitboard
#define square_bb(square) (1ULL << (square))

// rank, file, diagonal and antidiagonal associated with the square
#define rank(square) (0xFFULL << ((square) & 56))
#define file(square) (0x0101010101010101ULL << ((square) & 7))
u64 diagonal(int square);
u64 antidiagonal(int square);
// gets positive or negative portion of rank, file, diagonal, or antidiagonal
#define pos_ray(line, square) ((line) & (-2ULL << (square)))
#define neg_ray(line, square) ((line) & ((1ULL << (square)) - 1))

// bit operations
#define get_bit(bitboard, square) (bitboard & (1ULL << square))
#define set_bit(bitboard, square) (bitboard |= (1ULL << square))
#define pop_bit(bitboard, square) (bitboard &= ~(1ULL << square))

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

// shift function
u64 shift(u64 bb, enum direction dir);
// shift macros
#define north(b)      ((b << 8))
#define south(b)      ((b >> 8))
#define east(b)       ((b << 1) & ~file_a)
#define north_east(b) ((b << 9) & ~file_a)
#define south_east(b) ((b >> 7) & ~file_a)
#define west(b)       ((b >> 1) & ~file_h)
#define north_west(b) ((b << 7) & ~file_h)
#define south_west(b) ((b >> 9) & ~file_h)

// pretty print bitboard and squares
void print_bitboard(u64 bitboard);
void print_square(int square);

static inline int lsb(u64 bb)
{
#if   defined(__GNUC__)
	return (int) __builtin_ctzll(bb);
#elif defined(_MSC_VER)
	u32 i;
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
	u32 i;
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

#endif
