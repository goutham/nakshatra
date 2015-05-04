#ifndef BOOK_H
#define BOOK_H

#include "move.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

class Board;

namespace movegen {
class MoveGenerator;
}

class Book {
 public:
  Book(Variant variant, const std::string& book_file);
  Move GetBookMove(const Board& board) const;

 private:
  void LoadBook(Board* board,
                movegen::MoveGenerator* movegen);

  std::map<std::string, std::vector<Move> > book_;
  const std::string book_file_;
};

#endif
