#include "movegen.h"
#include "attacks.h"
#include "board.h"
#include "common.h"
#include "move_array.h"
#include "piece.h"
#include "side_relative.h"

#include <array>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

U64 AllAttacks(const Piece piece, const Board& board) {
  const U64 occupancy_bitboard = board.BitBoard();

  U64 piece_bitboard = board.BitBoard(piece);
  U64 attack_map = 0ULL;

  while (piece_bitboard) {
    const int lsb_index = Lsb1(piece_bitboard);
    attack_map |= attacks::Attacks(occupancy_bitboard, lsb_index, piece);
    piece_bitboard ^= (1ULL << lsb_index);
  }

  return attack_map;
}

enum PawnMoveType {
  NW_CAPTURE = 9,
  NE_CAPTURE = 7,
  ONE_STEP = 8,
  TWO_STEP = 16
};

template <Variant variant, Side side>
void AddPawnPromotions(const int from_index, const int to_index,
                       MoveArray* move_array) {
  auto add_move = [&from_index, &to_index,
                   &move_array](const Piece piece_type) {
    move_array->Add(Move(from_index, to_index, PieceOfSide(piece_type, side)));
  };

  add_move(BISHOP);
  add_move(KNIGHT);
  add_move(QUEEN);
  add_move(ROOK);

  if (variant == Variant::SUICIDE) {
    add_move(KING);
  }
}

template <Variant variant, Side side, PawnMoveType pawn_move_type>
void AddPawnMoves(U64 pawn_bitboard, MoveArray* move_array) {
  static_assert(side == Side::WHITE || side == Side::BLACK, "Invalid side");

  constexpr int add = side == Side::WHITE ? -pawn_move_type : pawn_move_type;

  while (pawn_bitboard) {
    const int lsb_index = Lsb1(pawn_bitboard);
    const int from_index = lsb_index + add;
    if (lsb_index <= 7 || lsb_index >= 56) {
      AddPawnPromotions<variant, side>(from_index, lsb_index, move_array);
    } else {
      move_array->Add(Move(from_index, lsb_index));
    }
    pawn_bitboard ^= (1ULL << lsb_index);
  }
}

template <Variant variant, Side side, PawnMoveType>
void AddPawnMoves(U64 pawn_bitboard, int* move_count) {
  constexpr int promotion_count =
      variant == Variant::NORMAL
          ? 4
          : (variant == Variant::SUICIDE ? 5 : throw std::logic_error(
                                                   "Unknown variant"));
  const U64 mask_7th_row = side_relative::MaskRow<side>(7);
  *move_count += PopCount(pawn_bitboard & mask_7th_row) * promotion_count +
                 PopCount(pawn_bitboard & ~mask_7th_row);
}

template <Side side>
U64 EnpassantBitBoard(const Board& board) {
  const int index = board.EnpassantTarget();
  return index == -1 ? 0ULL
                     : ((1ULL << index) & side_relative::MaskRow<side>(5));
}

template <Variant variant, Side side, typename MoveAccumulatorType>
void GeneratePawnMoves(const Board& board, const bool generate_captures_only,
                       MoveAccumulatorType move_acc) {
  const U64 opp_bitboard = board.BitBoard(OppositeSide(side));
  const U64 pawn_capturable = opp_bitboard | EnpassantBitBoard<side>(board);

  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  const U64 nw_captured =
      side_relative::PushNorthWest<side>(pawn_bitboard) & pawn_capturable;
  if (nw_captured) {
    AddPawnMoves<variant, side, NW_CAPTURE>(nw_captured, move_acc);
  }

  const U64 ne_captured =
      side_relative::PushNorthEast<side>(pawn_bitboard) & pawn_capturable;
  if (ne_captured) {
    AddPawnMoves<variant, side, NE_CAPTURE>(ne_captured, move_acc);
  }

  if (generate_captures_only) {
    return;
  }

  const U64 empty_bitboard = ~board.BitBoard();

  const U64 one_step =
      side_relative::PushFront<side>(pawn_bitboard) & empty_bitboard;
  const U64 two_step = side_relative::PushFront<side>(one_step) &
                       empty_bitboard & side_relative::MaskRow<side>(3);
  AddPawnMoves<variant, side, ONE_STEP>(one_step, move_acc);
  AddPawnMoves<variant, side, TWO_STEP>(two_step, move_acc);
}

template <Side side>
U64 PawnCaptures(const U64 pawn_bitboard, const U64 pawn_capturable_bitboard) {
  return pawn_capturable_bitboard &
         (side_relative::PushNorthWest<side>(pawn_bitboard) |
          side_relative::PushNorthEast<side>(pawn_bitboard));
}

// Castling is ignored.
template <Side side>
U64 GenerateAttackBitBoard(const Board& board) {
  constexpr Side opp_side = OppositeSide(side);

  const U64 opp_bitboard = board.BitBoard(opp_side);
  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, side));
  const U64 empty_bitboard = ~board.BitBoard();

  const U64 pawn_captures =
      PawnCaptures<side>(board.BitBoard(PieceOfSide(PAWN, side)),
                         opp_bitboard | EnpassantBitBoard<side>(board));
  const U64 pawn_one_step =
      side_relative::PushFront<side>(pawn_bitboard) & empty_bitboard;
  const U64 pawn_two_step = side_relative::PushFront<side>(pawn_one_step) &
                            empty_bitboard & side_relative::MaskRow<side>(3);

  auto attacks = [&board](const Piece piece_type) -> U64 {
    return AllAttacks(PieceOfSide(piece_type, side), board);
  };

  return pawn_captures | pawn_one_step | pawn_two_step | attacks(QUEEN) |
         attacks(ROOK) | attacks(BISHOP) | attacks(KNIGHT) | attacks(KING);
}

void BitBoardToMoves(const int index, U64 bitboard, MoveArray* move_array) {
  while (bitboard) {
    const int lsb_index = Lsb1(bitboard);
    move_array->Add(Move(index, lsb_index));
    bitboard ^= (1ULL << lsb_index);
  }
}

void BitBoardToMoves(const int unused_index, U64 bitboard, int* move_count) {
  *move_count += PopCount(bitboard);
}

// Assumes king is not in check.
template <Side side>
void GenerateCastlingMoves(const Board& board, const U64 opp_attack_map,
                           MoveArray* move_array) {
  constexpr Side opp_side = OppositeSide(side);
  const int row = (side == Side::WHITE) ? 0 : 7;

  auto is_square_empty = [&](const int row, const int col) {
    return board.PieceAt(row, col) == NULLPIECE;
  };

  const U64 pawn_bitboard = board.BitBoard(PieceOfSide(PAWN, opp_side));
  const U64 pawn_capturable_empty_squares =
      (side_relative::PushNorthWest<opp_side>(pawn_bitboard) |
       side_relative::PushNorthEast<opp_side>(pawn_bitboard)) &
      ~board.BitBoard();

  const U64 restricted_squares = opp_attack_map | pawn_capturable_empty_squares;

  if (board.CanCastle(side, KING)) {
    if (is_square_empty(row, FILE_F) && is_square_empty(row, FILE_G) &&
        !(restricted_squares & (SetBit(row, FILE_F) | SetBit(row, FILE_G)))) {
      move_array->Add(Move(INDX(row, FILE_E), INDX(row, FILE_G)));
    }
  }

  if (board.CanCastle(side, QUEEN)) {
    if (is_square_empty(row, FILE_C) && is_square_empty(row, FILE_D) &&
        is_square_empty(row, FILE_B) &&
        !(restricted_squares & (SetBit(row, FILE_C) | SetBit(row, FILE_D)))) {
      move_array->Add(Move(INDX(row, FILE_E), INDX(row, FILE_C)));
    }
  }
}

// Does not generate castling for king.
template <Variant variant, Side side, typename MoveAccumulatorType>
void GeneratePieceMoves(const Board& board, const Piece piece,
                        const bool generate_captures_only,
                        MoveAccumulatorType move_acc) {
  assert(PieceOfSide(piece, side) == piece);

  if (PieceType(piece) == PAWN) {
    GeneratePawnMoves<variant, side>(board, generate_captures_only, move_acc);
    return;
  }

  constexpr Side opp_side = OppositeSide(side);

  const U64 occupancy_bitboard = board.BitBoard();
  const U64 self_bitboard = board.BitBoard(side);
  const U64 opp_bitboard = board.BitBoard(opp_side);

  U64 piece_bitboard = board.BitBoard(piece);

  while (piece_bitboard) {
    const int lsb_index = Lsb1(piece_bitboard);
    U64 attack_map =
        attacks::Attacks(occupancy_bitboard, lsb_index, piece) & ~self_bitboard;
    if (generate_captures_only) {
      attack_map &= opp_bitboard;
    }
    if (attack_map) {
      BitBoardToMoves(lsb_index, attack_map, move_acc);
    }
    piece_bitboard ^= (1ULL << lsb_index);
  }
}

template <Side side>
bool Captures(const Board& board) {
  constexpr Side opp_side = OppositeSide(side);

  const U64 opp_bitboard = board.BitBoard(opp_side);

  auto captures = [&](const Piece piece_type) -> bool {
    return AllAttacks(PieceOfSide(piece_type, side), board) & opp_bitboard;
  };

  return captures(QUEEN) || captures(ROOK) || captures(BISHOP) ||
         captures(KNIGHT) || captures(KING) ||
         !!PawnCaptures<side>(board.BitBoard(PieceOfSide(PAWN, side)),
                              opp_bitboard | EnpassantBitBoard<side>(board));
}

template <Side side, typename MoveAccumulatorType>
void GenerateMoves_Suicide(const Board& board, MoveAccumulatorType move_acc) {
  const bool generate_captures_only = Captures<side>(board);

  auto generate = [&](const Piece piece_type) {
    GeneratePieceMoves<Variant::SUICIDE, side>(
        board, PieceOfSide(piece_type, side), generate_captures_only, move_acc);
  };

  generate(BISHOP);
  generate(KING);
  generate(KNIGHT);
  generate(PAWN);
  generate(QUEEN);
  generate(ROOK);
}

template <Side side>
void GenerateMoves_Normal(Board* board, MoveArray* move_array) {
  MoveArray pseudo_legal_move_array;

  auto generate = [&](const Piece piece_type) {
    GeneratePieceMoves<Variant::NORMAL, side>(
        *board, PieceOfSide(piece_type, side), false, &pseudo_legal_move_array);
  };

  generate(BISHOP);
  generate(KING);
  generate(KNIGHT);
  generate(PAWN);
  generate(QUEEN);
  generate(ROOK);

  constexpr Piece king_piece = PieceOfSide(KING, side);
  const int king_index = Lsb1(board->BitBoard(king_piece));

  assert(king_index >= 0);

  // Attacks on current playing side.
  const U64 opp_attack_map = GenerateAttackBitBoard<OppositeSide(side)>(*board);

  // If king is under check, work on evading check.
  if (opp_attack_map & board->BitBoard(king_piece)) {
    for (size_t i = 0; i < pseudo_legal_move_array.size(); ++i) {
      const Move& move = pseudo_legal_move_array.get(i);

      board->MakeMove(move);

      // New opponent attack map.
      const U64 new_opp_attack_map =
          GenerateAttackBitBoard<OppositeSide(side)>(*board);

      // Include move if legal.
      if (!(new_opp_attack_map & board->BitBoard(king_piece))) {
        move_array->Add(move);
      }

      board->UnmakeLastMove();
    }
  } else {
    if (board->CanCastle(side, KING) || board->CanCastle(side, QUEEN)) {
      GenerateCastlingMoves<side>(*board, opp_attack_map, move_array);
    }

    // Check if there are any pinned pieces. This is not an exact set of
    // pins but always a superset.
    const U64 potential_pins =
        attacks::Attacks(board->BitBoard(king_piece), king_index, QUEEN) &
        opp_attack_map & board->BitBoard(side);

    for (size_t i = 0; i < pseudo_legal_move_array.size(); ++i) {
      const Move& move = pseudo_legal_move_array.get(i);

      // Add all non-king, non-pinned piece moves.
      if (move.from_index() != king_index &&
          !((1ULL << move.from_index()) & potential_pins)) {
        move_array->Add(move);
        continue;
      }

      // For king moves and pinned pieces, make sure the moves are valid.
      board->MakeMove(move);

      const U64 new_opp_attack_map =
          GenerateAttackBitBoard<OppositeSide(side)>(*board);

      if (!(new_opp_attack_map & board->BitBoard(king_piece))) {
        move_array->Add(move);
      }

      board->UnmakeLastMove();
    }
  }
}

} // namespace

U64 ComputeAttackMap(const Board& board, const Side attacker_side) {
  U64 attack_map = 0ULL;
  switch (attacker_side) {
  case Side::WHITE:
    attack_map = GenerateAttackBitBoard<Side::WHITE>(board);
    break;

  case Side::BLACK:
    attack_map = GenerateAttackBitBoard<Side::BLACK>(board);
    break;

  default:
    throw std::runtime_error("Unknown side");
  }
  return attack_map;
}

void MoveGeneratorSuicide::GenerateMoves(MoveArray* move_array) {
  switch (board_.SideToMove()) {
  case Side::BLACK:
    GenerateMoves_Suicide<Side::BLACK>(board_, move_array);
    break;

  case Side::WHITE:
    GenerateMoves_Suicide<Side::WHITE>(board_, move_array);
    break;

  default:
    throw std::runtime_error("Unknown side");
  }
}

int MoveGeneratorSuicide::CountMoves() {
  int move_count = 0;

  switch (board_.SideToMove()) {
  case Side::BLACK:
    GenerateMoves_Suicide<Side::BLACK>(board_, &move_count);
    break;

  case Side::WHITE:
    GenerateMoves_Suicide<Side::WHITE>(board_, &move_count);
    break;

  default:
    throw std::runtime_error("Unknown side");
  }

  return move_count;
}

bool MoveGeneratorSuicide::IsValidMove(const Move& move) {
  MoveArray move_array;
  GenerateMoves(&move_array);
  return move_array.Contains(move);
}

void MoveGeneratorNormal::GenerateMoves(MoveArray* move_array) {
  switch (board_->SideToMove()) {
  case Side::BLACK:
    GenerateMoves_Normal<Side::BLACK>(board_, move_array);
    break;

  case Side::WHITE:
    GenerateMoves_Normal<Side::WHITE>(board_, move_array);
    break;

  default:
    throw std::runtime_error("Unknown side");
  }
}

int MoveGeneratorNormal::CountMoves() {
  MoveArray move_array;
  GenerateMoves(&move_array);
  return move_array.size();
}

bool MoveGeneratorNormal::IsValidMove(const Move& move) {
  MoveArray move_array;
  GenerateMoves(&move_array);
  return move_array.Contains(move);
}
