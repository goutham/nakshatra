#ifndef CREATION_H
#define CREATION_H

#include "board.h"
#include "common.h"
#include "config.h"
#include "egtb.h"
#include "eval.h"
#include "eval_antichess.h"
#include "eval_standard.h"
#include "extensions.h"
#include "iterative_deepener.h"
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

class PlayerBuilder {
public:
  PlayerBuilder(Variant variant)
      : variant_(variant), external_transpos_(false) {}

  virtual ~PlayerBuilder() {
    if (external_transpos_) {
      transpos_.release();
    }
  }

  virtual void BuildBoard() { board_.reset(new Board(variant_)); }
  virtual void BuildBoard(const std::string& fen) {
    board_.reset(new Board(variant_, fen));
  }
  virtual void BuildMoveGenerator() = 0;
  virtual void BuildMoveOrderer() = 0;
  virtual void BuildEGTB() = 0;
  virtual void BuildEvaluator() = 0;
  virtual void BuildTranspositionTable() = 0;

  virtual void InjectExternalTranspositionTable(TranspositionTable* transpos) {
    transpos_.reset(transpos);
    external_transpos_ = true;
  }
  virtual void BuildTimer() { timer_.reset(new Timer); }
  virtual void BuildIterativeDeepener() {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    assert(timer_ != nullptr);
    assert(transpos_ != nullptr);
    assert(eval_ != nullptr);
    root_move_orderer_.reset(new EvalScoreOrderer(board_.get(), eval_.get()));
    iterative_deepener_.reset(new IterativeDeepener(
        variant_, board_.get(), movegen_.get(), timer_.get(), transpos_.get(),
        root_move_orderer_.get(), egtb_.get()));
  }

  // BuildExtensions only allocates memory for the extensions_ object. The
  // contents of the object will be initialized only after AddExtensions is
  // called. While building our object hierarchy, BuildExtensions will be
  // called before building any other object and AddExtensions will be called
  // after building all other objects. This is because some extensions may
  // require arbitrarily many other objects to have been built. The
  // side-effect of this is that none of the constructors which take extensions_
  // as an argument can refer to its contents.
  virtual void BuildExtensions() { extensions_.reset(new Extensions()); }
  virtual void AddExtensions() {}

  virtual void BuildPlayer() = 0;

  virtual Player* GetPlayer() const { return player_.get(); }

  virtual MoveGenerator* GetMoveGenerator() const { return movegen_.get(); }

  virtual EGTB* GetEGTB() const { return egtb_.get(); }

  virtual Evaluator* GetEvaluator() const { return eval_.get(); }

  virtual TranspositionTable* GetTranspos() const { return transpos_.get(); }

  virtual Timer* GetTimer() const { return timer_.get(); }

protected:
  const Variant variant_;
  const std::map<std::string, std::string> config_map_;
  std::unique_ptr<Board> board_;
  std::unique_ptr<MoveGenerator> movegen_;
  std::unique_ptr<MoveOrderer> move_orderer_;
  std::unique_ptr<MoveOrderer> root_move_orderer_;
  std::unique_ptr<IterativeDeepener> iterative_deepener_;
  std::unique_ptr<Evaluator> eval_;
  std::unique_ptr<TranspositionTable> transpos_;
  std::unique_ptr<Timer> timer_;
  std::unique_ptr<Extensions> extensions_;
  std::unique_ptr<Player> player_;
  std::unique_ptr<EGTB> egtb_;
  bool external_transpos_;
};

class StandardPlayerBuilder : public PlayerBuilder {
public:
  StandardPlayerBuilder() : PlayerBuilder(Variant::STANDARD) {}

  void BuildTranspositionTable() {
    transpos_.reset(new TranspositionTable(STANDARD_TRANSPOS_SIZE));
  }

  void BuildMoveGenerator() override {
    assert(board_ != nullptr);
    movegen_.reset(new MoveGeneratorStandard(board_.get()));
  }

  void BuildMoveOrderer() override {
    assert(board_ != nullptr);
    move_orderer_.reset(new StandardMoveOrderer(board_.get()));
  }

  void BuildEGTB() override {}

  void BuildEvaluator() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    eval_.reset(new EvalStandard(board_.get(), movegen_.get()));
  }

  void BuildPlayer() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    assert(iterative_deepener_ != nullptr);
    assert(transpos_ != nullptr);
    assert(timer_ != nullptr);
    // For standard player, extensions_ could be NULL as of now.
    player_.reset(new Player(board_.get(), movegen_.get(),
                             iterative_deepener_.get(), transpos_.get(),
                             timer_.get(), egtb_.get(), extensions_.get()));
  }
};

class AntichessPlayerBuilder : public PlayerBuilder {
public:
  AntichessPlayerBuilder(const bool enable_pns = true)
      : PlayerBuilder(Variant::ANTICHESS), enable_pns_(enable_pns) {}

  void BuildTranspositionTable() {
    transpos_.reset(new TranspositionTable(ANTICHESS_TRANSPOS_SIZE));
  }

  void BuildMoveGenerator() override {
    assert(board_ != nullptr);
    movegen_.reset(new MoveGeneratorAntichess(*board_.get()));
  }

  void BuildMoveOrderer() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    move_orderer_.reset(new AntichessMoveOrderer(board_.get(), movegen_.get()));
  }

  void BuildEGTB() override {
    assert(board_ != nullptr);
    std::vector<std::string> egtb_filenames;
    assert(GlobFiles(ANTICHESS_EGTB_PATH_GLOB, &egtb_filenames));
    egtb_.reset(new EGTB(egtb_filenames, *board_.get()));
    egtb_->Initialize();
  }

  void BuildEvaluator() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    eval_.reset(new EvalAntichess(board_.get(), movegen_.get(), egtb_.get()));
  }

  void AddExtensions() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    assert(eval_ != nullptr);
    assert(extensions_ != nullptr);
    if (enable_pns_) {
      extensions_->pns_extension.pns_timer.reset(new Timer);
      extensions_->pns_extension.pn_search.reset(new PNSearch(
          board_.get(), movegen_.get(), eval_.get(), egtb_.get(),
          transpos_.get(), extensions_->pns_extension.pns_timer.get()));
    }
  }

  void BuildPlayer() override {
    assert(board_ != nullptr);
    assert(movegen_ != nullptr);
    assert(iterative_deepener_ != nullptr);
    assert(transpos_ != nullptr);
    assert(timer_ != nullptr);
    assert(extensions_ != nullptr);
    player_.reset(new Player(board_.get(), movegen_.get(),
                             iterative_deepener_.get(), transpos_.get(),
                             timer_.get(), egtb_.get(), extensions_.get()));
  }

private:
  const bool enable_pns_;
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
    player_builder_->BuildExtensions(); // Must always be called first.
    player_builder_->BuildMoveGenerator();
    player_builder_->BuildMoveOrderer();
    player_builder_->BuildEGTB();
    player_builder_->BuildEvaluator();
    player_builder_->BuildTimer();
    player_builder_->BuildIterativeDeepener();
    player_builder_->BuildPlayer();
    player_builder_->AddExtensions(); // Must always be called last.
    return player_builder_->GetPlayer();
  }

private:
  PlayerBuilder* player_builder_;
};

#endif
