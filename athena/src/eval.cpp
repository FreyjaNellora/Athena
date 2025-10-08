#include "eval.h"

// Material values (centipawns)
#include "position.h"
#include "chess.h"
#include <array>

namespace athena {

int evaluate(const Position& pos) {
    // Material-only evaluation for 4-player chess
    std::array<int, COLOR_NB> material{};

    for (Square sq : ALL_SQUARES) {
        const auto pc = pos.board[sq];
        const Color c = pc.color();
        if (c == None) continue;  // skip empty/invalid

        const auto pt = pc.piece();
        int val = 0;
        switch (pt) {
            case Pawn:   val = 100; break;
            case Knight: val = 300; break;
            case Bishop: val = 300; break;
            case Rook:   val = 500; break;
            case Queen:  val = 900; break;
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

    return stmMat - othersMat;

}

}