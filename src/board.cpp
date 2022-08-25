#include "board.h"
#include "common.h"
#include "fen.h"
#include "move.h"
#include "zobrist.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

namespace {

const std::map<const Variant, const std::string> variant_fen_map = {
    {Variant::STANDARD, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -"},
    {Variant::ANTICHESS, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - -"}};

// clang-format off
constexpr uint8_t CASTLING_MASKS[64] = {
    0b10, 0, 0, 0,   0b11, 0, 0,   0b1,
       0, 0, 0, 0,      0, 0, 0,     0,
       0, 0, 0, 0,      0, 0, 0,     0,
       0, 0, 0, 0,      0, 0, 0,     0,
       0, 0, 0, 0,      0, 0, 0,     0,
       0, 0, 0, 0,      0, 0, 0,     0,
       0, 0, 0, 0,      0, 0, 0,     0,
  0b1000, 0, 0, 0, 0b1100, 0, 0, 0b100,
};
// clang-format on

} // namespace

Board::Board(const Variant variant)
    : Board(variant, variant_fen_map.at(variant)) {}

Board::Board(const Variant variant, const std::string& fen) {
  castling_allowed_ = true;
  MoveStackEntry* top = move_stack_.Top();
  top->castle = 0xF;
  if (variant == Variant::ANTICHESS) {
    castling_allowed_ = false;
    top->castle = 0;
  }

  FEN::MakeBoardArray(fen, board_array_);
  side_to_move_ = FEN::PlayerToMove(fen);
  top->ep_index = FEN::EnpassantIndex(fen);
  if (castling_allowed_) {
    top->castle = FEN::CastlingAvailability(fen);
  }

  std::fill(std::begin(bitboard_sides_), std::end(bitboard_sides_), 0ULL);
  std::fill(std::begin(bitboard_pieces_), std::end(bitboard_pieces_), 0ULL);

  for (int i = 0; i < BOARD_SIZE; ++i) {
    Piece piece = board_array_[i];
    if (piece == NULLPIECE) {
      continue;
    }
    const U64 b = (1ULL << i);
    bitboard_sides_[SideIndex(PieceSide(piece))] |= b;
    bitboard_pieces_[PieceIndex(piece)] |= b;
  }

  top->zobrist_key = GenerateZobristKey();
}

void Board::MakeMove(const Move& move) {
  move_stack_.Push();

  const int from_index = move.from_index();
  const int to_index = move.to_index();
  const int from_row = ROW(from_index);
  const int from_col = COL(from_index);
  const int to_row = ROW(to_index);
  const int to_col = COL(to_index);
  const Piece src_piece = board_array_[from_index];
  const Piece dest_piece = board_array_[to_index];

  MoveStackEntry* top = move_stack_.Top();
  const MoveStackEntry* prev = move_stack_.Seek(1);
  top->move = move;
  top->captured_piece = dest_piece;
  top->zobrist_key = prev->zobrist_key;
  top->castle = prev->castle;
  top->half_move_clock = prev->half_move_clock + 1;

  // Remove piece at source square and destination square (if any).
  RemovePiece(from_index);
  if (dest_piece != NULLPIECE) {
    top->half_move_clock = 0; // captures reset half-move clock
    RemovePiece(to_index);
  }

  // Handle pawn move.
  if (PieceType(src_piece) == PAWN) {
    top->half_move_clock = 0; // pawn moves reset half-move clock
    top->ep_index = 0;

    if (abs(to_row - from_row) == 2) {
      // Two space pawn move.
      top->ep_index = INDX((to_row + from_row) >> 1, to_col);
    } else if (from_col != to_col && dest_piece == NULLPIECE) {
      // Enpassant capture.
      RemovePiece(INDX(from_row, to_col));
    }
    top->zobrist_key ^= zobrist::EP(prev->ep_index);
    top->zobrist_key ^= zobrist::EP(top->ep_index);

    PlacePiece(to_index, move.is_promotion()
                             ? PieceOfSide(move.promoted_piece(), side_to_move_)
                             : src_piece);

    FlipSideToMove();
    return;
  } else {
    top->ep_index = 0;
    top->zobrist_key ^= zobrist::EP(prev->ep_index);
    top->zobrist_key ^= zobrist::EP(top->ep_index);
  }

  // Handle castling.
  if (castling_allowed_) {
    if (PieceType(src_piece) == KING && abs(to_col - from_col) > 1) {
      const int rook_index = INDX(from_row, to_col > from_col ? 7 : 0);
      const Piece rook = board_array_[rook_index];
      RemovePiece(rook_index);
      PlacePiece(INDX(from_row, (to_col + from_col) >> 1), rook);
    }
    top->castle &= ~CASTLING_MASKS[from_index];
    top->castle &= ~CASTLING_MASKS[to_index];
    top->zobrist_key ^= zobrist::Castling(prev->castle);
    top->zobrist_key ^= zobrist::Castling(top->castle);
  }

  PlacePiece(to_index, src_piece);
  FlipSideToMove();
}

bool Board::UnmakeLastMove() {
  if (!move_stack_.Size()) {
    return false;
  }
  FlipSideToMove();

  const MoveStackEntry* top = move_stack_.Top();
  const Move& move = top->move;
  const int from_index = move.from_index();
  const int to_index = move.to_index();
  const int from_row = ROW(from_index);
  const int from_col = COL(from_index);
  const int to_col = COL(to_index);
  const Piece dest_piece = board_array_[to_index];

  if (move.is_promotion()) {
    RemovePieceNoZ(to_index);
    PlacePieceNoZ(from_index, PieceOfSide(PAWN, side_to_move_));
    if (top->captured_piece != NULLPIECE) {
      PlacePieceNoZ(to_index, top->captured_piece);
    }
    move_stack_.Pop();
    return true;
  }

  if (PieceType(dest_piece) == PAWN && to_col != from_col &&
      top->captured_piece == NULLPIECE) {
    PlacePieceNoZ(INDX(from_row, to_col),
                  PieceOfSide(PAWN, OppositeSide(side_to_move_)));
  } else if (castling_allowed_ && PieceType(dest_piece) == KING &&
             abs(to_col - from_col) == 2) {
    // Move rook.
    const int rook_old_index = INDX(from_row, to_col > from_col ? 7 : 0);
    const int rook_cur_index = INDX(from_row, (to_col + from_col) >> 1);
    const Piece rook = board_array_[rook_cur_index];
    RemovePieceNoZ(rook_cur_index);
    PlacePieceNoZ(rook_old_index, rook);
  }

  RemovePieceNoZ(to_index);
  PlacePieceNoZ(from_index, dest_piece);
  if (top->captured_piece != NULLPIECE) {
    PlacePieceNoZ(to_index, top->captured_piece);
  }

  move_stack_.Pop();
  return true;
}

void Board::MakeNullMove() {
  move_stack_.Push();
  MoveStackEntry* top = move_stack_.Top();
  const MoveStackEntry* prev = move_stack_.Seek(1);
  top->move = Move();
  top->captured_piece = NULLPIECE;
  top->ep_index = prev->ep_index;
  top->zobrist_key = prev->zobrist_key;
  top->castle = prev->castle;
  top->half_move_clock = 0;
  FlipSideToMove();
}

bool Board::UnmakeNullMove() {
  if (!move_stack_.Size()) {
    return false;
  }
  FlipSideToMove();
  move_stack_.Pop();
  return true;
}

bool Board::CanCastle(const Side side, const Piece piece_type) const {
  if (side == Side::NONE || (piece_type != KING && piece_type != QUEEN)) {
    throw std::runtime_error("Bad parameters");
  }

  const int shift =
      (side == Side::BLACK ? 2 : 0) + (piece_type == QUEEN ? 1 : 0);

  return move_stack_.Top()->castle & (1U << shift);
}

bool Board::CanCastle(Piece piece_type) const {
  return CanCastle(side_to_move_, piece_type);
}

std::string Board::ParseIntoFEN() const {
  const MoveStackEntry* top = move_stack_.Top();
  return FEN::MakeFEN(board_array_, side_to_move_, top->castle, top->ep_index);
}

void Board::DebugPrintBoard() const {
  using std::cout;
  using std::endl;

  static const char* white_back = "\x1b[47m";
  static const char* black_back = "\x1b[40;1m";
  static const char* white_piece = "\x1b[31;1m";
  static const char* black_piece = "\x1b[34m";

  cout << "##### Board Dump #####\n";
  for (int i = 7; i >= 0; --i) {
    cout << "# " << i + 1 << " ";
    for (int j = 0; j < 8; ++j) {
      if (i % 2 == 0) {
        if (j % 2 == 1) {
          cout << white_back;
        } else {
          cout << black_back;
        }
      } else {
        if (j % 2 == 0) {
          cout << white_back;
        } else {
          cout << black_back;
        }
      }
      const char piece = PieceToChar(board_array_[INDX(i, j)]);
      if (isupper(piece)) {
        cout << white_piece;
      } else {
        cout << black_piece;
      }
      cout << " " << piece << " ";
      cout << "\x1b[0m"; // clear side attributes
    }
    cout << endl;
  }
  cout << "#   ";
  for (int i = 0; i < 8; ++i) {
    cout << " " << (char)(i + 65) << " ";
  }
  cout << endl;
  cout << "# W pieces: " << NumPieces(Side::WHITE) << endl;
  cout << "# B pieces: " << NumPieces(Side::BLACK) << endl;
  cout << "# Zobrist key: " << move_stack_.Top()->zobrist_key << endl;
  cout << "# FEN: " << ParseIntoFEN() << endl;
  cout << "# EP index: " << move_stack_.Top()->ep_index << endl;
  cout << "##### End Board Dump #####" << endl;
}

void Board::FlipSideToMove() {
  side_to_move_ = (side_to_move_ == Side::WHITE ? Side::BLACK : Side::WHITE);
  move_stack_.Top()->zobrist_key ^= zobrist::Turn();
}

U64 Board::GenerateZobristKey() {
  U64 zkey = 0;
  for (int i = 0; i < BOARD_SIZE; ++i) {
    if (IsValidPiece(board_array_[i])) {
      Piece piece = board_array_[i];
      zkey ^= zobrist::Get(piece, i);
    }
  }
  if (SideToMove() == Side::BLACK) {
    zkey ^= zobrist::Turn();
  }
  zkey ^= zobrist::EP(move_stack_.Top()->ep_index);
  zkey ^= zobrist::Castling(move_stack_.Top()->castle);
  return zkey;
}

void Board::PlacePiece(const int index, const Piece piece) {
  board_array_[index] = piece;
  const U64 bit_mask = (1ULL << index);
  bitboard_sides_[SideIndex(PieceSide(piece))] |= bit_mask;
  bitboard_pieces_[PieceIndex(piece)] |= bit_mask;
  move_stack_.Top()->zobrist_key ^= zobrist::Get(piece, index);
}

void Board::PlacePieceNoZ(const int index, const Piece piece) {
  board_array_[index] = piece;
  const U64 bit_mask = (1ULL << index);
  bitboard_sides_[SideIndex(PieceSide(piece))] |= bit_mask;
  bitboard_pieces_[PieceIndex(piece)] |= bit_mask;
}

void Board::RemovePiece(const int index) {
  const Piece piece = board_array_[index];
  board_array_[index] = NULLPIECE;
  const U64 bit_mask = ~(1ULL << index);
  bitboard_sides_[SideIndex(PieceSide(piece))] &= bit_mask;
  bitboard_pieces_[PieceIndex(piece)] &= bit_mask;
  move_stack_.Top()->zobrist_key ^= zobrist::Get(piece, index);
}

void Board::RemovePieceNoZ(const int index) {
  const Piece piece = board_array_[index];
  board_array_[index] = NULLPIECE;
  const U64 bit_mask = ~(1ULL << index);
  bitboard_sides_[SideIndex(PieceSide(piece))] &= bit_mask;
  bitboard_pieces_[PieceIndex(piece)] &= bit_mask;
}
