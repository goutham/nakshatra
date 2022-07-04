#ifndef CREATION_H
#define CREATION_H

#include "board.h"
#include "common.h"
#include "config.h"
#include "egtb.h"
#include "eval.h"
#include "eval_antichess.h"
#include "eval_standard.h"
#include "move_order.h"
#include "movegen.h"
#include "player.h"
#include "pn_search.h"
#include "search_algorithm.h"
#include "timer.h"
#include "transpos.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

struct BuildOptions {
  // Initial FEN to construct board with. If this is empty, use default
  // initialization for given variant.
  std::string init_fen;

  // Use this transposition table instead of building a new one. This is useful
  // during pondering where we need a new Player instance to ponder over a
  // different board position but reuse the existing transposition table. If
  // this variable is nullptr, a new transposition table is built.
  TranspositionTable* transpos = nullptr;
};

template <typename T>
T* RawPtr(std::unique_ptr<T>& o, bool check_not_null = true) {
  if (check_not_null) {
    assert(o.get());
  }
  return o.get();
}

class PlayerBuilder {
public:
  const Variant variant;
  std::unique_ptr<Board> board;
  std::unique_ptr<MoveGenerator> movegen;
  std::unique_ptr<Evaluator> evaluator;
  std::unique_ptr<TranspositionTable> transpos;
  std::unique_ptr<Timer> timer;
  std::unique_ptr<Player> player;
  std::unique_ptr<EGTB> egtb;
  bool external_transpos;

  PlayerBuilder(Variant variant) : variant(variant), external_transpos(false) {}

  virtual ~PlayerBuilder() {
    if (external_transpos) {
      transpos.release();
    }
  }

  virtual void BuildBoard() { board.reset(new Board(variant)); }
  virtual void BuildBoard(const std::string& fen) {
    board.reset(new Board(variant, fen));
  }
  virtual void BuildMoveGenerator() = 0;
  virtual void BuildEGTB() = 0;
  virtual void BuildEvaluator() = 0;
  virtual void BuildTranspositionTable() = 0;

  virtual void
  InjectExternalTranspositionTable(TranspositionTable* ext_transpos) {
    transpos.reset(ext_transpos);
    external_transpos = true;
  }
  virtual void BuildTimer() { timer.reset(new Timer); }

  virtual void BuildPlayer() = 0;
};

class StandardPlayerBuilder : public PlayerBuilder {
public:
  StandardPlayerBuilder() : PlayerBuilder(Variant::STANDARD) {}

  void BuildTranspositionTable() {
    transpos.reset(new TranspositionTable(STANDARD_TRANSPOS_SIZE));
  }

  void BuildMoveGenerator() override {
    movegen.reset(new MoveGeneratorStandard(RawPtr(board)));
  }

  void BuildEGTB() override {}

  void BuildEvaluator() override {
    evaluator.reset(new EvalStandard(RawPtr(board), RawPtr(movegen)));
  }

  void BuildPlayer() override {
    player.reset(new Player(variant, RawPtr(board), RawPtr(movegen),
                            RawPtr(transpos), RawPtr(evaluator), RawPtr(timer),
                            egtb.get()));
  }
};

class AntichessPlayerBuilder : public PlayerBuilder {
public:
  AntichessPlayerBuilder() : PlayerBuilder(Variant::ANTICHESS) {}

  void BuildTranspositionTable() {
    transpos.reset(new TranspositionTable(ANTICHESS_TRANSPOS_SIZE));
  }

  void BuildMoveGenerator() override {
    movegen.reset(new MoveGeneratorAntichess(*RawPtr(board)));
  }

  void BuildEGTB() override {
    std::vector<std::string> egtb_filenames;
    assert(GlobFiles(ANTICHESS_EGTB_PATH_GLOB, &egtb_filenames));
    egtb.reset(new EGTB(egtb_filenames, *RawPtr(board)));
    egtb->Initialize();
  }

  void BuildEvaluator() override {
    evaluator.reset(
        new EvalAntichess(RawPtr(board), RawPtr(movegen), RawPtr(egtb)));
  }

  void BuildPlayer() override {
    player.reset(new Player(variant, RawPtr(board), RawPtr(movegen),
                            RawPtr(transpos), RawPtr(evaluator), RawPtr(timer),
                            egtb.get()));
  }
};

class PlayerBuilderDirector {
public:
  PlayerBuilderDirector(PlayerBuilder* player_builder)
      : player_builder_(player_builder) {}

  Player* Build(const BuildOptions& options) {
    if (options.transpos) {
      player_builder_->InjectExternalTranspositionTable(options.transpos);
    } else {
      player_builder_->BuildTranspositionTable();
    }
    if (options.init_fen.empty()) {
      player_builder_->BuildBoard();
    } else {
      player_builder_->BuildBoard(options.init_fen);
    }
    player_builder_->BuildMoveGenerator();
    player_builder_->BuildEGTB();
    player_builder_->BuildEvaluator();
    player_builder_->BuildTimer();
    player_builder_->BuildPlayer();
    return player_builder_->player.get();
  }

private:
  PlayerBuilder* player_builder_;
};

#endif
