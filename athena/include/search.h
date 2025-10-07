#pragma once

#include "position.h"  // Position
#include "thread.h"    // Thread
#include "chess.h"     // Move

namespace athena {

// Search constants (defined in search.cpp)
extern int  SCORE_INFINITY;
extern int  SCORE_DRAW;
extern int  SCORE_CHECKMATE;
extern int  MAX_PLAY;

// Sentinel/root result moves (defined in search.cpp)
extern Move MOVE_DRAW_FIFTY_MOVE;
extern Move MOVE_CHECKMATE;
extern Move MOVE_STALEMATE;

// Core search entry
int negamax(Position& pos, Thread& thread, int alpha, int beta, int depth, int play = 0);

} // namespace athena

