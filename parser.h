#ifndef PARSER_H
#define PARSER_H

#include "array.h"

struct pgn_tag {
	char *name, *desc;
};

struct pgn_move {
	char text[8];	// move encoding in SAN
	int  nag;	// 0-255 NAG value (optional)
	char *comment;  // comment (optional)
};

struct pgn {
	struct array tags;	// all tags in parsed order
	struct array moves;	// all moves (white and black) in order
	char result[8];		// result of the game
};

void pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif

