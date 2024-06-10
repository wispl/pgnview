#ifndef PGN_MOVELIST_H
#define PGN_MOVELIST_H

#include "array.h"
#include "movegen.h"

int santogenc(char *text, struct movegenc *conf);
void pgn_movelist(struct array *pgn_moves, struct array *movelist);

#endif
