#ifndef MOVE_H
#define MOVE_H

#include <stdbool.h>
#include <stdint.h>

// 6 bits for from square
// 6 bits for to square
// 1 bit for capture flag
// 1 bit for promotion flag
// 2 bits for special flags (promo piece or castling)
typedef uint16_t move;

#define make_move(from, to, capture, promo, special) \
	((from) + ((to) << 6) + ((capture) << 12) + ((promo) << 13) + ((special) << 14))
#define make_quiet(from, to) (make_move((from), (to), false, false, 0))
#define make_capture(from, to) (make_move((from), (to), true, false, 0))
#define make_castle(from, to) (make_move((from), (to), false, false, true))
#define make_promotion(from, to, is_capture, piece) \
	(make_move((from), (to), (is_capture), true, (piece)))

#define move_from(move)          ((move) & 0x3f)
#define move_to(move)            (((move) >> 6) & 0x3f)
#define move_promo_piece(move)   (((move) >> 14) & 0x3)

#define move_set_from(move, from)         ((move) |= ((from) & 0x3f))
#define move_set_to(move, to)             ((move) |= (((to) & 0x3f) << 6))
#define move_set_promo_piece(move, piece) ((move) |= (((piece) & 0x3) << 13))

#define move_is_capture(move)    (((move) >> 12) & 1)
#define move_is_promotion(move)  (((move) >> 13) & 1)
#define move_is_quiet(move)      (!move_is_promotion((move)) && !move_is_capture((move)))
#define move_is_castle(move)     (move_is_quiet((move)) && move_promo_piece((move)))

#endif
