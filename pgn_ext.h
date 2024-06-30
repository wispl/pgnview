#ifndef PGN_EXT_H
#define PGN_EXT_H

#include "pgn.h"
#include "types.h"

// Converts a list of "struct pgn_move" to a list of "struct move", "moves"
// must be large enough! (>= pgn_moves.len).
// Returns the number of moves filled.
int pgn_to_moves(const struct pgn *pgn, move *moves);

#endif
