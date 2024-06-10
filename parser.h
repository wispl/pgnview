#ifndef PARSER_H
#define PARSER_H

#include "array.h"

struct pgn_tag {
	char *name, *desc;
};
array_define(pgn_taglist, struct pgn_tag)

struct pgn_move {
	char text[8];	// move encoding in SAN
	int  nag;	// 0-255 NAG value (optional)
	char *comment;  // comment (optional)
};
array_define(pgn_movelist, struct pgn_move)

struct pgn {
	struct pgn_taglist  tags;	// all tags in parsed order
	struct pgn_movelist moves;	// all moves (white and black) in order
	char result[8];			// result of the game
};

void pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif

