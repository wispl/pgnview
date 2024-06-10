#include "pgn_movelist.h"

#include "bitboard.h"
#include "array.h"
#include "movegen.h"
#include "parser.h"

#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static enum piece chrtopiece(char c)
{
	switch (c) {
	case 'K': return KING;
	case 'Q': return QUEEN;
	case 'R': return ROOK;
	case 'B': return BISHOP;
	case 'N': return KNIGHT;
	}
	return PAWN;
}

static inline int ranktoi(char c)
{
	return (c - '1') * 8;
}

static inline int filetoi(char c)
{
	return c - 'a';
}

static inline int santoi(char *san)
{
	return filetoi(san[0]) + ranktoi(san[1]);
}

// Extracts from, to, and piece type from a SAN with special characters removed
// and returns the disambiguation square, rank or file. Disambiguation is 
// needed in cases where two or more pieces can reach the same square
//
// QUIET: 	e4, Nf6
// CAPTURES:	exd4->ed4, Nxf6->Nf6
// PROMOTIONS:  ed4=Q->ed4
static int extract_san(char *text, int len, struct movegenc *conf)
{
	assert(len >= 2 && len <= 5);

	// piece type is first char
	conf->piece    = chrtopiece(text[0]);
	// destination square occurs two chars from end
	conf->target   = square_bb(santoi(&text[len - 2]));

	switch (len) {
	// Pawn moves: e5, d5 -> e5, d5, these don't need disambiguation
	case 2: return -1;
	// Non-ambiguous moves: Nf6, Bg6 -> Nf6, Bg6
	// isupper is for pawn captures: exd4 -> (e)d4
	// otherwise it is just a regular move/capture with no ambiguation
	case 3: return isupper(text[0]) ? -1 : filetoi(text[0]);
	// Ambiguous moves: Ngf6, R2f2 -> N(g)f6, R(2)f2
	// These are moves which require a rank or file to indiciate a piece,
	// such as when two rooks are on the same rank
	case 4: return isalpha(text[1]) ? filetoi(text[1]) : ranktoi(text[1]);
	// Ex Ambiguous moves: Ng8f6 -> N(g8)f6
	// These not very common but may occur when you have, for
	// example, three rooks attacking one square
	case 5: return santoi(&text[1]);
	}
	return -1;
}

int santogenc(char *text, struct movegenc *conf)
{
	if (strcmp(text, "0-0-0") == 0) {
		// TODO
		return KING;
	}

	if (strcmp(text, "0-0") == 0) {
		// TODO
		return KING;
	}

	// captures
	char* x_start  = strchr(text, 'x');
	// promotions
	char* eq_start = strchr(text, '=');

	int len = strlen(text);

	if (eq_start) {
		len -= 2;
		conf->movetype = PROMOTION;
	}
	if (x_start) {
		len -= 1;
		// get rid of the 'x', making it just a regular quiet move
		// exd5 -> ed5, Nxf6 -> Nf6
		text[x_start - text] 	 = text[1];
		text[x_start - text + 1] = text[2];
		conf->movetype = CAPTURE;
		
	} else {
		conf->movetype = QUIET;
	}

	return extract_san(text, len, conf);
}

static bool find_move(struct board *board, struct movegenc *conf, int disamb, struct move *move)
{
	struct array moves;
	array_init(&moves, sizeof(struct move));
	generate_moves(board, &moves, conf);

	if (moves.len == 1) {
		// move = &array_get(&moves, struct move, 0);
		memcpy(move, &array_get(&moves, struct move, 0), sizeof(struct move));
		array_free(&moves);
		return true;
	}

	for (int i = 0; i < moves.len; ++i) {
		struct move m = array_get(&moves, struct move, i);
		bool match = (disamb == m.from) ||
			     (disamb < 8 && (disamb == (m.from & 7))) ||
			     (disamb == m.from / 8);
		if (match) {
			move = &m;
			array_free(&moves);
			return true;
		}
	}
	return false;
}

void pgn_movelist(struct array *pgn_moves, struct array *movelist)
{
	struct board board;
	board_init(&board);

	struct movegenc conf;
	struct move move;
	for (int i = 0; i < pgn_moves->len; ++i) {
		struct pgn_move pgn_move = array_get(pgn_moves, struct pgn_move, i);
		conf.color = (i & 1);
		int disamb = santogenc(pgn_move.text, &conf);
		bool found = find_move(&board, &conf, disamb, &move);

		if (found) {
			array_push(movelist, move);
			board_move(&board, &move);
		}
	}
}