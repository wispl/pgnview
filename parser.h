#ifndef PARSER_H
#define PARSER_H

#include "list.h"

struct tag {
	struct node node;
	char *type, *description;
};

struct move {
	struct node node;
	char *white, *black;
	char *black_comment, *white_comment;
};

struct pgn {
	struct node tags;
	struct node moves;
};

void pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif

