#ifndef PARSER_H
#define PARSER_H

#include "list.h"

struct pgn_tag {
	struct node node;
	char *name, *desc;
};

struct pgn_move {
	struct node node;
	char text[8];	// move encoding in SAN
	int  nag;	// 0-255 NAG value (optional)
	char *comment;  // comment (optional)
};

struct pgn {
	struct node tags;
	struct node moves;
};

void pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif

