#ifndef MOVGEN_H
#define MOVGEN_H

#include "board.h"
#include "move.h"
#include "move_array.h"
#include "piece.h"

#include <stdexcept>

namespace movegen {

// Interface implemented by the move generators of all variants. Access to any
// other objects (such as the Board) can be provided through the constructor of
// the derived classes.
class MoveGenerator {
 public:
  virtual ~MoveGenerator() {}

  // Generates all variant specific legal moves for the current board position
  // and populates the passed in 'move_array' object.
  virtual void GenerateMoves(MoveArray* move_array) = 0;

  // Returns true if the move is valid in the variant for the current board
  // position.
  virtual bool IsValidMove(const Move& move) = 0;

 protected:
  MoveGenerator() {}
};

// Populates the move_array with all possible moves from given index to various
// squares on the board given by set bits in the bitboard.
void BitBoardToMoveList(const int index,
                        const U64 bitboard,
                        MoveArray* move_array);

// Computes all possible attacks on the board by the attacking side.
U64 ComputeAttackMap(const Board& board, const Side attacker_side);

// Computes attack bitboard for given non-pawn piece from given index.
U64 Attacks(const Piece piece, const int index, const Board& board);

// Computes attacks for all non-pawn pieces of respective type and side and
// returns their ORed U64.
U64 AllAttacks(const Piece piece, const Board& board);

// Computes pawn captures for all pawns of input side.
U64 AllPawnCaptures(const Side side, const Board& board);

// Computes move count for given side in suicide chess.
int CountMoves(const Side side, const Board& board);

// Helper function to add pawn promotions to move array.
template <typename Container>
void AddPawnPromotions(const Container& promotions,
                       const int from_index,
                       const int to_index,
                       MoveArray* move_array) {
  for (const Piece piece : promotions) {
    move_array->Add(Move(from_index, to_index, piece));
  }
}

// Returns U64 with bits in the given column set. Columns 0..7 correspond to
// files A..H.
constexpr U64 MaskColumn(const int column) {
  return (column >= 0 && column <= 7)
             ? (0x0101010101010101ULL << (7 - column))
             : throw std::logic_error("Invalid column");
}

constexpr U64 MaskRow(const int row) {
  return (row >= 0 && row <= 7) ? (0x00000000000000FFULL << (8 * row))
                                : throw std::logic_error("Invalid row");
}

}  // namespace movegen

#endif
