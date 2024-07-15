#include "types.h"

#include <stdio.h>

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
			printf(" %c", bitboard & (1ULL << square) ? '1' : '.');
		}
		printf("\n");
	}
	printf("\n   a b c d e f g h\n");
}
