#ifndef MOVGEN_H
#define MOVGEN_H

#include "board.h"
#include "move.h"
#include "move_array.h"
#include "piece.h"

// Interface implemented by the move generators of all variants.
class MoveGenerator {
public:
  virtual ~MoveGenerator() {}

  virtual void GenerateMoves(MoveArray* move_array) = 0;

  virtual int CountMoves() = 0;

  virtual bool IsValidMove(const Move& move) = 0;

protected:
  MoveGenerator() {}
};

// Move generator for standard chess.
class MoveGeneratorStandard : public MoveGenerator {
public:
  MoveGeneratorStandard(Board* board) : board_(board) {}

  void GenerateMoves(MoveArray* move_array) final;

  int CountMoves() final;

  bool IsValidMove(const Move& move) final;

private:
  Board* board_;
};

// Move generator for antichess.
class MoveGeneratorAntichess : public MoveGenerator {
public:
  MoveGeneratorAntichess(const Board& board) : board_(board) {}

  void GenerateMoves(MoveArray* move_array) final;

  int CountMoves() final;

  bool IsValidMove(const Move& move) final;

private:
  const Board& board_;
};

// Computes all possible attacks on the board by the attacking side.
U64 ComputeAttackMap(const Board& board, const Side attacker_side);

#endif
