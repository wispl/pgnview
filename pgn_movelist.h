#ifndef PGN_MOVELIST_H
#define PGN_MOVELIST_H

#include "move.h"
#include "movegen.h"
#include "parser.h"

int santogenc(char *text, struct movegenc *conf);
void pgn_movelist(struct pgn_movelist *pgn_moves, struct movelist *movelist);

#endif
