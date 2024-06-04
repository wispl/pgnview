#include "bitboard.h"

#include <assert.h>
#include <stdio.h>

// bitscans
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

int lsb(u64 bb)
{
	const u64 debruijn64 = 0x03f79d71b4cb0a89ULL;
	assert (bb != 0);
	return index64[((bb ^ (bb-1)) * debruijn64) >> 58];
}

int msb(u64 bb)
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

int pop_lsb(u64 bb)
{
	int i = lsb(bb);
	bb &= bb - 1;
	return i;
}

u64 diagonal(int square)
{
   const u64 main_diagonal = 0x8040201008040201;
   int diag = 8 * (square & 7) - (square & 56);
   int north = -diag & ( diag >> 31);
   int south =  diag & (-diag >> 31);
   return (main_diagonal >> south) << north;
}

u64 antidiagonal(int square)
{
   const u64 main_diagonal = 0x0102040810204080;
   int diag = 56 - 8 * (square & 7) - (square & 56);
   int north = -diag & ( diag >> 31);
   int south =  diag & (-diag >> 31);
   return (main_diagonal >> south) << north;
}

u64 shift(u64 bb, enum direction dir)
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

void print_bitboard(u64 bitboard)
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
