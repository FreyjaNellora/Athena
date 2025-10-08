#include "eval.h"

// Material values (centipawns)
#include "position.h"
#include "chess.h"
#include <array>

namespace athena {

int evaluate(const Position& pos) {
	constexpr std::array<int, PIECE_NB> pieceValue = {0, 300, 300, 500, 900, 100, 0, 0};
	// King=0, Knight=300, Bishop=300, Rook=500, Queen=900, Pawn=100, Empty=0, Stone=0

	std::array<int, COLOR_NB> material{};

	for (Square sq : ALL_SQUARES) {
		PieceClass pc = pos.board[sq];
		Piece pt = pc.piece();
		Color c = pc.color();
		if (pt == Empty || pt == Stone || c == None) continue;
		material[c] += pieceValue[pt];
	}

	Color stm = pos.states.back().turn;
	int stmMat = material[stm];
	int othersMat = 0;
	for (Color c : COLORS) {
		if (c != stm) othersMat += material[c];
	}
	return stmMat - othersMat;
}

}