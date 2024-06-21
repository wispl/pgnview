#ifndef MOVE_H
#define MOVE_H

enum movetype {
	QUIET,        // movement only, no material is altered
	CAPTURE,      // material is decreased for opposing side
	PROMOTION,    // material is added for promoting side
	CASTLE,
};

struct move {
	enum movetype movetype;
	int from;
	int to;
};

#endif
