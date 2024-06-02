#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "list.h"

// Standards for .pgn can be found here
// https://ia802908.us.archive.org/26/items/pgn-standard-1994-03-12/PGN_standard_1994-03-12.txt

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

struct parser {
	bool unhandled_error;
	struct pgn *pgn;
	struct lexer lexer;
};

struct pgn* pgn_read(char *filename);
void pgn_free(struct pgn *pgn);

#endif

