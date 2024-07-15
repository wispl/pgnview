#include "pgn_ext.h"

#include "pgn.h"
#include "chess.h"

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

static enum piece chrtopiece(const char c)
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

static inline int ranktoi(const char c)
{
	return (c - '1') * 8;
}

static inline int filetoi(const char c)
{
	return c - 'a';
}

static inline int santoi(const char *san)
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
static int extract_san(const char *from, const char *to, struct movegenc *conf)
{
	int len = to - from;
	assert(len >= 0 && len <= 4);

	// skip the 'x'
	if (conf->type == CAPTURE || conf->type == PROMO_CAPTURE)
		++to;

	// first character indicates the piece type, or a pawn if lowercase
	conf->piece  = chrtopiece(from[0]);
	conf->target = square_bb(santoi(to));

	switch (len) {
	// Pawn moves: e5, d5, these never need disambiguation
	case 0: return -1;
	// Non-ambiguous moves: (N)f6, (B)g6, (e)xd4
	case 1: return -1;
	// Ambiguous moves: (Ng)f6, (R2)f2
	// These moves require either a rank or file to identify a piece, such
	// as when two rooks are on the same rank.
	case 2: return isalpha(from[1]) ? filetoi(from[1]) : ranktoi(from[1]);
	// Extra Ambiguous moves: (Ng8)f6
	// Moves which require a square to identify a piece. Not very common.
	case 3: return santoi(&from[1]);
	}
	return -1;
}

// Wrapper for extract_san, handles special cases like castling, promotion and
// checks/mates.
// Fills out 'info' with information parsed from text.
static void get_moveinfo(char *text, enum color color, struct moveinfo *info)
{
	info->conf.color = color;
	info->conf.type = QUIET;

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

	// checks and mates, ignore for now, no usable information
	char end = text[len - 1];
	if (end == '+' || end == '#')
		--len;

	// promotions, for example, fxg8(=Q)
	char *eq_start = strchr(text, '=');
	if (eq_start) {
		info->conf.type = PROMOTION;
		info->promo_piece = chrtopiece(text[len - 1]);
		len -= 2;
	}

	char *to = &text[len - 2];
	char *x_start  = strchr(text, 'x');
	if (x_start) {
		to = x_start;
		info->conf.type = (eq_start) ? PROMO_CAPTURE : CAPTURE;
	}

	info->hint = extract_san(text, to, &info->conf);
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
		// Multiple promotion moves, but we already have the piece we
		// want so set it manually and return early
		if (move_is_promotion(move)) {
			move_set_promo_piece(move, info->promo_piece - 1);
			return move;
		}
		if (disambiguate(info->hint, move_from(move)))
			return move;
	}
	return 0;
}

int pgn_to_moves(const struct pgn *pgn, move *moves)
{
	if (!attacks_table_initilized())
		init_lineattacks_table();

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
