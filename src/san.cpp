#include "board.h"
#include "common.h"
#include "move.h"
#include "move_array.h"
#include "movegen.h"
#include "u64_op.h"
#include "san.h"

#include <string>
#include <cstdlib>

using std::string;

string SAN(const Board& board, const Move& move) {
  const Piece piece = board.PieceAt(move.from_index());

  // Castling.
  if (PieceType(piece) == KING) {
    const int squares_moved =
        abs(static_cast<int>(COL(move.to_index())) -
            static_cast<int>(COL(move.from_index())));
    if (squares_moved == 2) {
      return "O-O";  // king side castling
    } else if (squares_moved == 3) {
      return "O-O-O";  // queen side castling
    }
  }

  string san;

  // Second part of the move is always the destination square. An 'x' is
  // prefixed in case of capture. Pawn promotions are handled later.
  string second_part;
  if (board.PieceAt(move.to_index()) != NULLPIECE ||
      (PieceType(piece) == PAWN &&
       COL(move.to_index()) != COL(move.from_index()))) {
    second_part.append("x");
  }
  second_part.append(Move::file_rank(move.to_index()));

  if (PieceType(piece) != PAWN) {
    const U64 dest_sq_bb = (1ULL << move.to_index());
    const U64 piece_bb = board.BitBoard(piece);

    san.append(1, PieceToChar(PieceType(piece)));

    // Iterate over all the pieces of the same type and check if there are
    // multiple pieces that can occupy the destination square.
    U64Op op(piece_bb);
    int index;
    std::vector<int> indices;
    while ((index = op.NextRightMostBitIndex()) != -1) {
      const U64 attacks = Attacks(piece, index, board);
      if (attacks & dest_sq_bb) {
        indices.push_back(index);
      }
    }
    // Disambiguate the source piece from other pieces that can move to the same
    // destination. In order of priority, we use file, then rank, then both file
    // and rank combined.
    bool disambiguate_by_file = false;
    bool disambiguate_by_rank = false;
    bool disambiguate_by_file_rank = false;
    if (indices.size() > 1) {
      disambiguate_by_file = true;
    }
    for (int index : indices) {
      if (index == move.from_index()) {
        continue;
      }
      if (COL(index) == COL(move.from_index())) {
        disambiguate_by_rank = true;
      }
      if (ROW(index) == ROW(move.from_index()) && disambiguate_by_rank) {
        disambiguate_by_file_rank = true;
      }
    }
    // Go in the reverse order of priority as they will only be set if higher
    // priority disambiguation doesn't work.
    if (disambiguate_by_file_rank) {
      san.append(Move::file_rank(move.from_index()));
    } else if (disambiguate_by_rank) {
      san.append(1, Move::rank(move.from_index()));
    } else if (disambiguate_by_file) {
      san.append(1, Move::file(move.from_index()));
    }
    san.append(second_part);
  } else {
    // For pawns, in case of capture, the move should be denoted as
    // <from-file>x<to-square>. For non-captures, it is simply <to-square>.
    if (COL(move.from_index()) != COL(move.to_index())) {
      san.append(1, Move::file(move.from_index()));
    }
    san.append(second_part);

    // Append promoted piece if any.
    if (move.promoted_piece() != NULLPIECE) {
      san.append(1, PieceToChar(PieceType(move.promoted_piece())));
    }
  }
  return san;
}

Move SANToMove(const string& move_san,
               const Board& board,
               MoveGenerator* movegen) {
  MoveArray move_array;
  movegen->GenerateMoves(&move_array);
  for (int i = 0; i < move_array.size(); ++i) {
    const Move& move = move_array.get(i);
    if (SAN(board, move) == move_san) {
      return move;
    }
  }
  throw std::runtime_error("Unknown move " + move_san);
}
