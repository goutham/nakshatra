#include "board.h"
#include "book.h"
#include "common.h"
#include "move.h"
#include "movegen.h"
#include "movegen_normal.h"
#include "movegen_suicide.h"
#include "san.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <time.h>
#include <vector>

Book::Book(Variant variant, const std::string& book_file)
    : book_file_(book_file) {
  switch (variant) {
    case NORMAL:
    {
      Board board(NORMAL);
      MoveGeneratorNormal movegen(&board);
      LoadBook(&board, &movegen);
      break;
    }
    case SUICIDE:
    {
      Board board(SUICIDE);
      MoveGeneratorSuicide movegen(board);
      LoadBook(&board, &movegen);
      break;
    }
    default:
      throw std::runtime_error("Unknown variant.");
  }
}

Move Book::GetBookMove(const Board& board) const {
  std::string fen = board.ParseIntoFEN();
  const auto book_entry = book_.find(fen);
  if (book_entry != book_.end()) {
    srand(time(NULL));
    std::cout << "# Book moves:\n";
    int i = 1;
    for (const Move& move : book_entry->second) {
      std::cout << "# " << i << ". " << SAN(board, move) << std::endl;
      ++i;
    }
    int r = rand() % book_entry->second.size();
    return book_entry->second.at(r);
  }
  // Return invalid move if no move is available in the book.
  return Move();
}

void Book::LoadBook(Board* board, MoveGenerator* movegen) {
  std::ifstream ifs;
  ifs.open(book_file_, std::ios::in);
  std::stringstream ss;
  ss << ifs.rdbuf();
  std::string contents(ss.str());
  ifs.close();

  std::stack<std::string> book_lines;
  for (int i = 0; i < contents.size();) {
    if (contents.at(i) == '(') {
      book_lines.push(std::string(1, contents.at(i)));
      ++i;
    } else if (contents.at(i) == ')') {
      while (book_lines.top() != "(") {
        book_lines.pop();
        board->UnmakeLastMove();
      }
      book_lines.pop();
      ++i;
    } else if (isalnum(contents.at(i))) {
      std::string move_san;
      while (i < contents.size() &&
             (isalnum(contents.at(i)) || contents.at(i) == '-')) {
        move_san.push_back(contents.at(i));
        ++i;
      }
      book_lines.push(move_san);
      Move move = SANToMove(move_san, *board, movegen);
      assert(move.is_valid());
      if (contents.at(i) != '^') {
        book_[board->ParseIntoFEN()].push_back(move);
      }
      board->MakeMove(move);
    } else {
      ++i;
    }
  }
}
