#ifndef ZOBRIST_H
#define ZOBRIST_H

#include "common.h"
#include "piece.h"

#define PIECE_MAX 7
#define COLOR_MAX 3
#define SQUARE_MAX 64

namespace zobrist {

void InitializeIfNeeded();

void PrintZobrist();

U64 Get(Piece piece, int sq);

U64 Turn();

U64 EP(int sq);

U64 Castling(unsigned char castle);

}

#endif
