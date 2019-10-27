#ifndef FEN_H
#define FEN_H

#include "piece.h"

#include <string>
#include <vector>

namespace FEN {

void MakeBoardArray(const std::string& fen, Piece board_array[]);

Side PlayerToMove(const std::string& fen);

unsigned char CastlingAvailability(const std::string& fen);

int EnpassantIndex(const std::string& fen);

std::string MakeFEN(const Piece board_array[], Side player_move,
                    const unsigned char castled, int ep_index);
} // namespace FEN

#endif
