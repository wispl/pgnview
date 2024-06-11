#include "bitboard.h"

#include <assert.h>
#include <stdio.h>

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

void print_square(int square)
{
	char str[3];
	str[0] = 'a' + (square & 7);
	str[1] = '1' + (square / 8);
	str[2] = '\0';
	printf("%s", str);
}
