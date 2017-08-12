#ifndef MOVE_H
#define MOVE_H

#include "piece.h"

#include <cassert>
#include <string>

// Space efficient move encoding which may be used in data structures such as
// the transposition table where storage efficiency is critical.
// Bit 0-5: From Square
// Bit 6-11: To Square
// Bit 12: 0 if White, 1 if Black Promoted Piece
// Bit 13-15: Promoted piece. 000 if there is no promotion, in which case
//            Bit 12 is irrelevant.
//
// Special case(s):
//   Denotes invalid move if all bits are set to 0. This works because no
//   valid move has same From and To Squares.
typedef unsigned short int EncodedMove;

class Move {
public:
  Move() : encoded_move_(0) {}

  Move(const int from_index, const int to_index) {
    EncodeMove(from_index, to_index, NULLPIECE);
  }

  Move(const int from_index, const int to_index, const Piece promoted_piece) {
    EncodeMove(from_index, to_index, promoted_piece);
  }

  Move(std::string m) {
    const int from_index = index(m.substr(0, 2));
    const int to_index = index(m.substr(2, 2));
    Piece promoted_piece = NULLPIECE;
    if (isalpha(m[4])) {
      promoted_piece = CharToPiece(m[4]);
    }
    EncodeMove(from_index, to_index, promoted_piece);
  }

  EncodedMove encoded_move() const { return encoded_move_; }

  int from_index() const { return encoded_move_ >> 10; }

  int to_index() const { return (encoded_move_ & 0x3FF) >> 4; }

  Piece promoted_piece() const {
    const int side = ((encoded_move_ & 0x8) ? -1 : 1);
    return (encoded_move_ & 0x7) * side;
  }

  bool operator==(const Move& move) const {
    // All bits except the 'side' bit of promoted piece are relevant.
    return (encoded_move_ & 0xFFF7) == (move.encoded_move() & 0xFFF7);
  }

  bool operator!=(const Move& move) const {
    // All bits except the 'side' bit of promoted piece are relevant.
    return (encoded_move_ & 0xFFF7) != (move.encoded_move() & 0xFFF7);
  }

  bool is_promotion() const { return encoded_move_ & 0x7; }

  bool is_valid() const { return encoded_move_ != 0; }

  std::string str() const {
    std::string d;
    Piece promoted = promoted_piece();
    if (promoted != NULLPIECE) {
      d = tolower(PieceToChar(promoted));
    }
    return file_rank(from_index()) + file_rank(to_index()) + d;
  }

  static int index(const std::string& s) {
    const int col = tolower(s[0]) - 'a';
    const int row = CharToDigit(s[1]) - 1;
    return INDX(row, col);
  }

  static char file(const int index) {
    return static_cast<char>(COL(index) + 97);
  }

  static char rank(const int index) { return DigitToChar(ROW(index) + 1); }

  static std::string file_rank(const int index) {
    char str[3] = {file(index), rank(index), '\0'};
    return static_cast<std::string>(str);
  }

private:
  void EncodeMove(const int from_index, const int to_index,
                  const Piece promoted_piece) {
    encoded_move_ = ((from_index << 10) | (to_index << 4) |
                     ((PieceSide(promoted_piece) == Side::BLACK) << 3) |
                     PieceType(promoted_piece));
  }

  EncodedMove encoded_move_;
};

#endif
