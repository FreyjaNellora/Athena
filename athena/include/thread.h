#pragma once

#include <cstdint>
#include <vector>
#include "chess.h"

namespace athena
{

class Thread
{
public:
    int score = 0;                      // root score from search
    Move move{};                        // best root move
    std::uint64_t nodes = 0;            // total nodes visited in this search
    std::vector<Move> pv;               // principal variation line
};

} // namespace athena
