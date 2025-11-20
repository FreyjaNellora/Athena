#include "search.h"
#include "movegen.h"
#include "thread.h"
#include "eval.h"
#include "chess.h"
#include "position.h"
#include <vector>
#include <algorithm>


namespace athena {

// Search score constants and limits:
// SCORE_INFINITY: upper bound for alpha-beta window; no reachable score exceeds this.
// SCORE_CHECKMATE: returned when checkmate is imminent (just below infinity).
// SCORE_DRAW: returned for draw conditions (fifty-move rule, stalemate).
// MAX_PLAY: maximum ply depth to guard against infinite recursion (256 plies ≈ 128 moves).
// MOVE_*: sentinel Move objects used to indicate game-end conditions (draw, checkmate, stalemate).
int SCORE_INFINITY  = 100000;
int SCORE_DRAW      = 0;
int SCORE_CHECKMATE = 99999;
int MAX_PLAY        = 256;
Move MOVE_DRAW_FIFTY_MOVE, MOVE_CHECKMATE, MOVE_STALEMATE;

// MVV-LVA (Most Valuable Victim - Least Valuable Attacker) style material lookup.
// Used for move ordering: higher scores move captures that win material earlier in search.
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

// Fail-hard quiescence search: extends the search only for captures to avoid horizon effects.
// Uses isRoyalSafe() to skip illegal moves that expose the king.
// Returns best score found within [alpha, beta); uses beta cutoff for alpha-beta pruning.
static int quiesce(Position& pos, int alpha, int beta) {
    // Evaluate current position (stand-pat).
    // If eval ≥ beta, we have a cutoff: this line is good enough to refute the parent move.
    int standPat = evaluate(pos);
    if (standPat >= beta) return beta;
    if (standPat >  alpha) alpha = standPat;

    // Generate and search all captures (noisy moves).
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
        // Fail-hard: update alpha if score improves, but never exceed beta.
        if (score > alpha) alpha = score;
        
    }
    return alpha;
}

// Recursive negamax with alpha-beta pruning.
// Implements depth-first search: decrements depth each ply.
// Calls quiesce() at leaf nodes (depth ≤ 0) to stabilize evaluation.
// Uses MAX_PLAY as a safeguard against infinite recursion.
// Sets thread.move and thread.score at root (play == 0) when a better move is found.
int negamax(Position& pos, Thread& thread, int alpha, int beta, int depth, int play) {
    // Base case: depth ≤ 0 or MAX_PLAY safety limit reached; enter quiescence search.
    // depth decremented each ply; play only guards MAX_PLAY (safety cap)
    if (depth <= 0 || play >= MAX_PLAY)
        return quiesce(pos, alpha, beta);

    const GameState& gs = pos.states.back();
    // Fifty-move rule: draw if clock ≥ 100 half-moves (50 full moves without capture or pawn move).
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

    // Move ordering using MVV-LVA: captures sorted by material gain (victim value - attacker value).
    // Quiet moves score 0; captures score 10000 + gain. Stable sort preserves move generator order.
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
            // At root (play == 0), record the best move and score for engine output.
            if (play == 0) {
                thread.score = bestScore;
                thread.move  = m;
            }
        }
        // Fail-hard beta cutoff: if score ≥ beta, return immediately (prune remaining moves).
        if (score >= beta) return beta;
        
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