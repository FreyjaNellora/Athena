#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "chess.h"
#include "position.h"

namespace athena
{

bool isSquareAttacked(const Position& pos, Square source, Color color) noexcept;
bool isRoyalSafe(const Position& pos, Color color) noexcept;
//
bool isSquareAttacked(const Position& pos, Square source, Color color) noexcept;

//
bool isRoyalSafe(const Position& pos, Color color) noexcept;

// Move generation
int genAllNoisyMoves(const Position& pos, Move* moves);
int genAllQuietMoves(const Position& pos, Move* moves);

} // namespace athena

#endif // #ifndef MOVEGEN_H