#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdlib.h>

// stretchy buffer from skeeto's growable-buf
#define VEC_INIT_SIZE 8
struct vec {
	int size;
	int len;
	char buffer[];
};

#define containerof(ptr) ((struct vec *)((char *)(ptr) - offsetof(struct vec, buffer)))
#define vec_init(vec)    (vec = 0)
#define vec_free(vec)                             \
	do {                                      \
		if ((vec)) {                      \
			free(containerof((vec))); \
				(vec) = 0;        \
		}                                 \
	} while (0)
#define vec_size(vec) ((vec) ? containerof((vec))->size : 0)
#define vec_len(vec)  ((vec) ? containerof((vec))->len : 0)
#define vec_last(vec) ((vec)[vec_len((vec)) == 0 ? 0 : (vec_len((vec)) - 1)])
#define vec_pop(vec)  ((vec)[--containerof((vec))->len])
#define vec_push(vec, e)                                         \
	do {                                                     \
		if (vec_len((vec)) == vec_size((vec)))           \
			(vec) = vec_grow((vec), sizeof(*(vec))); \
		(vec)[containerof((vec))->len++] = (e);          \
	} while (0)

static void* vec_grow(void *v, int element_size)
{
	struct vec *vec;
	if (v) {
		vec = containerof(v);
		vec->size += (vec->size == 0) ? VEC_INIT_SIZE : vec->size;
		vec = realloc(vec, sizeof(struct vec) + element_size * vec->size);
		if (!vec)
			abort();
	} else {
		vec = malloc(sizeof(struct vec) + element_size * VEC_INIT_SIZE);
		if (!vec)
			abort();
		vec->size = VEC_INIT_SIZE;
		vec->len = 0;
	}
	return vec->buffer;
}

/// pgn standard: https://ia802908.us.archive.org/26/items/pgn-standard-1994-03-12/PGN_standard_1994-03-12.txt

/// This file handles parsing of pgn files, returning a parsed
/// structure containing moves, tags, and nags. Examine the link
/// above for more details on a pgn file format. A brief description
/// is included in some of the structures.

/// The result of a pgn read
enum pgn_result {
	PGN_OK,               // All good :)
	PGN_FILE_ERROR,       // Unable to open or read file
	PGN_LEX_ERROR,        // Unable to parse file, unknown syntax error 
	PGN_TAG_PARSE_ERROR,  // Could not parse tag, syntax error
	PGN_MOVE_PARSE_ERROR, // Could not parse move, syntax error
};

enum game_result {
	WHITE_WIN,  // 1-0
	BLACK_WIN,  // 0-1
	DRAW,       // 1/2-1/2
	UNKNOWN     // *
};

/// A pgn tag gives metadata about the game, here are a couple
/// of example tags for a game
///
///   [Event "IBM Man-Machine, New York USA"]
///   [Site "01"]
///   [Date "1997.??.??"]
///   [EventDate "?"]
///   [Round "?"]
///   [Result "1-0"]
///   [White "Garry Kasparov"]
///   [Black "Deep Blue (Computer)"]
///   [ECO "A06"]
///   [WhiteElo "?"]
///   [BlackElo "?"]
///   [PlyCount "89"]
///
/// The first word denotes the key and the second, wrapped in
/// quotes, denotes the data. You can define whatever tag  
/// you want, but you have to follow STR (Seven Tag Roster)
/// which means the following tags must be included
///   1) Event (the name of the tournament or match event)
///   2) Site (the location of the event)
///   3) Date (the starting date of the game)
///   4) Round (the playing round ordinal of the game)
///   5) White (the player of the white pieces)
///   6) Black (the player of the black pieces)
///   7) Result (the result of the game)
/// The order of the tags does not matter, except when
/// when exporting a game, in which case follow the order listed
struct pgn_tag {
	char *name, *desc;
};

/// A ply is half of a move, in other words, a move consists of
/// two plies, one from white, and one from black. Each ply
/// can be annotated with a comment or NAG if desired. A NAG
/// is a numerical code with a defined meaning (see link above).
/// The ply text is the ply encoded in SAN, standard algebraic 
/// notation. See the below for an example of a ply
///
///  Bb5 {Ruy Lopez Opening} $1
///
/// This ply has text Bb5, with the comment Ruy Lopez opening
/// and a NAG of 1 which corresponds to a "good move".
struct pgn_ply {
	char text[8];	// move encoding in SAN
	int  nag;	// 0-255 NAG value (optional)
	char *comment;  // comment (optional)
};

/// This holds all tags and plies of a game
struct pgn_game {
	enum pgn_result result;       // whether the parsed file failed or not 
	struct pgn_tag *tags;	      // all tags in parsed order 
	struct pgn_ply *plies;	      // all plies in parsed order
	enum game_result termination; // result of the game (termination marker)
};

/// A pgn can hold multiple games
struct pgn {
	struct pgn_game *games; // all games in parsed order
	char* filename;         // filename of the parsed file
	int number_errors;      // number of games with errors 
};

enum pgn_result pgn_read(struct pgn *pgn, char *filename);
void pgn_free(struct pgn *pgn);

#endif
