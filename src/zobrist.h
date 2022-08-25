#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "common.h"

namespace zobrist {

void PrintZobrist();

U64 Get(Piece piece, int sq);

U64 Turn();

U64 EP(int sq);

U64 Castling(unsigned char castle);
} // namespace zobrist

#endif
