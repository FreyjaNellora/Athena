
#include "eval.h"
#include "position.h"
#include "chess.h"
#include "movegen.h"
#include "utility.h"
#include <array>

namespace athena {

static inline int count_legal_moves_for(Position const& pos, Color who) {
    Position tmp = pos;                       // cheap copy for turn tweak
    tmp.states.back().turn = who;             // set side to move
    Move moves[MAX_MOVES];
    int sz = 0;
    sz += genAllNoisyMoves(tmp, moves + sz);
    sz += genAllQuietMoves(tmp, moves + sz);
    return sz;
}

int evaluate(const Position& pos) {
    // ---- material ----
    std::array<int, COLOR_NB> material{};

    for (Square sq : ALL_SQUARES) {
        const auto pc = pos.board[sq];
        const Color c  = pc.color();
        if (c == None) continue;  // empty/invalid

        const auto pt = pc.piece();
        int val = 0;
        switch (pt) {
            case Pawn:   val = 100; break;
            case Knight: val = 300; break;
            case Bishop: val = 300; break;
            case Rook:   val = 500; break;
            case Queen:  val = 900; break;
            // no king/empty/stone in material
            case King:
            case Empty:
            case Stone:
            default:     val = 0;   break;
        }
        material[c] += val;
    }

    const Color stm = pos.states.back().turn;

    int total = 0;
    for (int m : material) total += m;

    const int stmMat    = material[stm];
    const int othersMat = total - stmMat;

    // ---- mobility (lightweight) ----
    constexpr int mobilityWeight = 1;  // keep tiny; material should dominate
    const int myMoves = count_legal_moves_for(pos, stm);

    int oppMoves = 0;
    for (int c = 0; c < COLOR_NB; ++c) {
        if (static_cast<Color>(c) == stm) continue;
        oppMoves += count_legal_moves_for(pos, static_cast<Color>(c));
    }

    const int mobility = mobilityWeight * (myMoves - oppMoves);

    return (stmMat - othersMat) + mobility;
}

} // namespace athena
