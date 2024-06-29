#include "pgn_ext.h"

#include "board.h"
#include "move.h"
#include "movegen.h"
#include "pgn.h"

#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Information needed to find a move based on SAN
struct moveinfo {
	// passed into generate_moves(), this helps eliminate most legal moves
	struct movegenc conf;
	// promoted piece if any
	enum piece promo_piece;
	// disambiguation hint, can be a square, file, or rank.
	int hint;
};

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

// Extracts from, to, and piece type from a SAN to a movegenc and returns the
// disambiguation square, rank or file. Disambiguations are needed in cases where
// two or more pieces can reach the same square. Note special characters like
// 'x' for caputures or '=', '+', '#' are ignored as we only care about squares
// here, not note movetype
//
// QUIET: 	e4, Nf6
// CAPTURES:	exd4->ed4, Nxf6->Nf6
// PROMOTIONS:  ed4=Q->ed4
static int extract_san(char *text, int len, struct movegenc *conf)
{
	assert(len >= 2 && len <= 5);

	// piece type is first char, lowercase means it is a pawn
	conf->piece    = chrtopiece(text[0]);
	// destination square occurs two chars from end
	conf->target   = square_bb(santoi(&text[len - 2]));

	switch (len) {
	// Pawn moves: e5, d5 -> e5, d5, these don't need disambiguation
	case 2: return -1;
	// Non-ambiguous moves: Nf6, Bg6 -> Nf6, Bg6
	// isupper is special case for pawn captures: exd4 -> (e)d4
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

// Wrapper for extract_san, handles special cases like castling, promotion and
// checks/mates.
// Fills out 'info' with information parsed from text.
static void get_moveinfo(char *text, enum color color, struct moveinfo *info)
{
	info->conf.color = color;

	bool short_castle = (strcmp(text, "O-O") == 0);
	bool long_castle  = (strcmp(text, "O-O-O") == 0);
	if (short_castle || long_castle) {
		info->conf.type    = CASTLE;
		info->conf.piece   = KING;
		info->conf.target  = (short_castle) ? h1 : a1;
		// flip sides if black
		info->conf.target += (color * a8);
		info->conf.target  = square_bb(info->conf.target);
		info->hint = -1;
		return;
	}

	int len = strlen(text);

	// checks and mates
	char end = text[len - 1];
	if (end == '+' || end == '#')
		--len;

	// captures and promotions
	char* x_start  = strchr(text, 'x');
	char* eq_start = strchr(text, '=');

	if (eq_start) {
		len -= 2;
		info->conf.type = PROMOTION;
		info->promo_piece = chrtopiece(text[len]);
	}
	if (x_start) {
		len -= 1;
		// get rid of the 'x', making it just a regular quiet move
		// exd5 -> ed5, Nxf6 -> Nf6
		int i = x_start - text;
		text[i]     = text[i + 1];
		text[i + 1] = text[i + 2];
		text[i + 2] = '\0';
		info->conf.type = CAPTURE;
		
	} else {
		info->conf.type = QUIET;
	}

	info->hint = extract_san(text, len, &info->conf);
}

// Checks if a file, a rank, or both file or rank matches 'from' based on
// 'disamb', the disambiguation.
static bool disambiguate(int hint, int from)
{
	return hint == from
	    || (hint < 8 && hint == (from & 7))
	    || hint == (from / 8);
}

static move find_move(struct board *board, struct moveinfo *info)
{
	move moves[16];
	move *last = generate_moves(board, moves, &info->conf);
	int len = last - moves;

	if (len == 1)
		return moves[0];

	for (int i = 0; i < len; ++i) {
		move move = moves[i];
		if (disambiguate(info->hint, move_from(move))) {
			if (move_is_promotion(move))
				move_set_promo_piece(move, info->promo_piece - 1);
			return move;
		}
	}
	return 0;
}

int pgn_to_moves(const struct pgn *pgn, move *moves)
{
	int n = 0;
	struct board board;
	board_init(&board);

	struct moveinfo info;
	for (int i = 0; i < pgn->movecount; ++i) {
		// white moves are even and black moves are odd, 0-based index
		enum color color = (i & 1);
		get_moveinfo(pgn->moves[i].text, color, &info);
		move move = find_move(&board, &info);

		if (move) {
			++n;
			moves[i] = move;
			board_move(&board, move);
		}
	}
	return n;
}
