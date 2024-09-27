#ifndef MOVGEN_H
#define MOVGEN_H

#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"

template <Variant variant>
  requires(IsStandard(variant))
MoveArray GenerateMoves(Board& board);

template <Variant variant>
  requires(IsAntichessLike(variant))
MoveArray GenerateMoves(Board& board);

template <Variant variant>
  requires(IsStandard(variant))
int CountMoves(Board& board);

template <Variant variant>
  requires(IsAntichessLike(variant))
int CountMoves(Board& board);

bool IsValidMove(Variant variant, Board& board, Move move);

// Computes all possible attacks on the board by the attacking side.
U64 ComputeAttackMap(const Board& board, Side attacker_side);

#endif
