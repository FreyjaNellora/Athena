#include "search.h"
#include "movegen.h"
#include "thread.h"
#include "eval.h"
#include "chess.h"
#include "position.h"
#include <vector>
#include <algorithm>


namespace athena {

int SCORE_INFINITY  = 100000;
int SCORE_DRAW      = 0;
int SCORE_CHECKMATE = 99999;
int MAX_PLAY        = 256;
Move MOVE_DRAW_FIFTY_MOVE, MOVE_CHECKMATE, MOVE_STALEMATE;

static inline int pieceValue(Piece p) {
    switch (p) {
        case Pawn:   return 100;
        case Knight: return 300;
        // Bishop = 300 for now (baseline)
        case Bishop: return 300;
        case Rook:   return 500;
        case Queen:  return 900;
        default:     return 0;
    }
}

static int quiesce(Position& pos, int alpha, int beta) {
    int standPat = evaluate(pos);
    if (standPat >= beta) return beta;
    if (standPat >  alpha) alpha = standPat;

    Move moves[MAX_MOVES];
    int size = 0;
    size += genAllNoisyMoves(pos, moves + size);

    for (int i = 0; i < size; ++i) {
        Move m = moves[i];
        pos.makemove(m);
        if (!isRoyalSafe(pos, pos.states.back().turn)) {
            pos.undomove(m);
            continue;
        }
        int score = -quiesce(pos, -beta, -alpha);

        if (score > alpha) alpha = score;
        
    }
    return alpha;
}

int negamax(Position& pos, Thread& thread, int alpha, int beta, int depth, int play) {
    // depth decremented each ply; play only guards MAX_PLAY (safety cap)
    if (depth <= 0 || play >= MAX_PLAY)
        return quiesce(pos, alpha, beta);

    const GameState& gs = pos.states.back();
    if (gs.clock >= 100) {
        if (play == 0) {
            thread.score = SCORE_DRAW;
            thread.move  = MOVE_DRAW_FIFTY_MOVE;
        }
        return SCORE_DRAW;
    }

    Move moves[MAX_MOVES];
    int size = 0;
    size += genAllNoisyMoves(pos, moves + size);
    size += genAllQuietMoves(pos, moves + size);

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
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const auto& a, const auto& b){ return a.first > b.first; });

    bool anyLegal = false;
    int bestScore = -SCORE_INFINITY;

    for (const auto& it : ordered) {
        Move m = it.second;
        pos.makemove(m);
        if (!isRoyalSafe(pos, pos.states.back().turn)) {
            pos.undomove(m);
            continue;
        }
        anyLegal = true;
        int score = -negamax(pos, thread, -beta, -alpha, depth - 1, play + 1);
        pos.undomove(m);
        if (score > bestScore) {
            bestScore = score;
            if (play == 0) {
                thread.score = bestScore;
                thread.move  = m;
            }
        }
        
    }
    if (!anyLegal) {
        if (isRoyalSafe(pos, pos.states.back().turn)) {
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