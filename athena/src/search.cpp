
#include "search.h"
#include "movegen.h"
#include "thread.h"
#include "eval.h"
#include "chess.h"      // Move, Piece, flags
#include "position.h"   // Position, GameState
#include <vector>
#include <algorithm>
#include "constants.h" // For constants, if needed

using namespace athena;

namespace athena {


int SCORE_INFINITY  = 100000;
int SCORE_DRAW      = 0;
int SCORE_CHECKMATE = 99999;   // just below infinity
int MAX_PLAY        = 256;

Move MOVE_DRAW_FIFTY_MOVE;
Move MOVE_CHECKMATE;
Move MOVE_STALEMATE;

int negamax(Position& pos, Thread& thread, int alpha, int beta, int depth, int play)
{
    // Base case: stop at the requested depth
        // We decrement depth each ply; stop when (depth <= 0)
        if (depth <= 0 || play >= MAX_PLAY)
    return athena::evaluate(pos);

    const GameState& gs = pos.states.back();

    // Fifty-move rule
    if (gs.clock > 50) {
        if (play == 0) {
            thread.score = SCORE_DRAW;
            thread.move  = MOVE_DRAW_FIFTY_MOVE;
        }
        return SCORE_DRAW;
    }

    // Generate moves
    Move moves[MAX_MOVES];
    int size = 0;
    size += athena::genAllNoisyMoves(pos, moves + size);
    size += athena::genAllQuietMoves(pos, moves + size);

    // Order moves: simple MVV-LVA for noisy moves, quiet = 0
    auto pieceValue = [](Piece p) {
        switch (p) {
            case Pawn:   return 100;
            case Knight: return 300;
            case Bishop: return 300;
            case Rook:   return 500;
            case Queen:  return 900;
            default:     return 0;
        }
    };

    std::vector<std::pair<int, Move>> ordered;
    ordered.reserve(size);
    for (int i = 0; i < size; ++i) {
        Move m = moves[i];
        int sc = 0;
        if (m.flag() == Noisy) {
            sc = 10000
               + pieceValue(pos.board[m.target()].piece())
               - pieceValue(pos.board[m.source()].piece());
        }
        ordered.emplace_back(sc, m);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });

    bool anyLegal = false;
    int bestScore = -SCORE_INFINITY;

    for (const auto& it : ordered) {
        Move m = it.second;

        pos.makemove(m);
    if (!athena::isRoyalSafe(pos, pos.states.back().turn)) {
            pos.undomove(m);
            continue;
        }
        anyLegal = true;

        // We use play+1 because the base case is (play == depth)
            // We decrement depth each ply; stop when (depth <= 0)
            int score = -negamax(pos, thread, -beta, -alpha, depth - 1, play + 1);
        pos.undomove(m);

        if (score > bestScore) {
            bestScore = score;
            if (play == 0) {
                thread.score = bestScore;
                thread.move  = m;
            }
        }
        if (score >= beta)   return score; // fail-hard beta cutoff
        if (score > alpha)   alpha = score;
    }

    // No legal moves: stalemate/checkmate
    if (!anyLegal) {
        if (athena::isRoyalSafe(pos, pos.states.back().turn)) {
            if (play == 0) { thread.score = SCORE_DRAW; thread.move = MOVE_STALEMATE; }
            return SCORE_DRAW;
        } else {
            if (play == 0) { thread.score = SCORE_CHECKMATE; thread.move = MOVE_CHECKMATE; }
            return SCORE_CHECKMATE;
        }
    }

    return bestScore;
}

} // namespace athena