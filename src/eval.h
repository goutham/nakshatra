#ifndef EVAL_H
#define EVAL_H

class Board;

class Evaluator {
public:
  virtual ~Evaluator() {}

  // Returns the score for current position of the board.
  virtual int Evaluate(int alpha, int beta) = 0;

  // Returns WIN, -WIN or DRAW if game is over; else returns -1.
  virtual int Result() const = 0;

protected:
  Evaluator() {}
};

#endif
