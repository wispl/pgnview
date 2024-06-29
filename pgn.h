#ifndef PARSER_H
#define PARSER_H

enum pgn_result {
	PGN_OK,
	PGN_FILE_ERROR,
	PGN_LEX_ERROR,
	PGN_TAG_PARSE_ERROR,
	PGN_MOVE_PARSE_ERROR,
};

struct pgn_tag {
	char *name, *desc;
};

struct pgn_move {
	char text[8];	// move encoding in SAN
	int  nag;	// 0-255 NAG value (optional)
	char *comment;  // comment (optional)
};

// TODO: technically the moves are plies, rename?
struct pgn {
	struct pgn_tag  *tags;	// all tags in parsed order
	int tagcount;
	struct pgn_move *moves;	// all moves (white and black) in parsed order
	int movecount;
	char result[8];		// result of the game
};

enum pgn_result pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif
