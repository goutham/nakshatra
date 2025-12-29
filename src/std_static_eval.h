#ifndef STATIC_EVAL_H
#define STATIC_EVAL_H

#include "attacks.h"
#include "board.h"
#include "common.h"
#include "pawns.h"
#include "pst.h"
#include "std_eval_params.h"
#include <array>
#include <iostream>

namespace standard {

inline constexpr int GAME_PHASE_INC[7] = {0, 0, 4, 2, 1, 1, 0};

template <Piece piece, typename ValueType>
void AddPSTScores(const StdEvalParams<ValueType>& params, U64 bb,
                  int& game_phase, ValueType& mgame_score,
                  ValueType& egame_score) {
  constexpr Piece piece_type = PieceType(piece);
  constexpr Side side = PieceSide(piece);
  const auto& pst_mgame = params.pst_mgame[piece_type];
  const auto& pst_egame = params.pst_egame[piece_type];
  while (bb) {
    const int sq = Lsb1(bb);
    int index = sq;
    if constexpr (side == Side::WHITE) {
      index = index ^ 56;
    }
    mgame_score += pst_mgame[index] + params.pv_mgame[piece_type];
    egame_score += pst_egame[index] + params.pv_egame[piece_type];
    game_phase += GAME_PHASE_INC[piece_type];
    bb ^= (1ULL << sq);
  }
}

struct PawnStructData {
  U64 pawn_zobrist_key;
  U64 white_bb;
  U64 black_bb;
  int w_mgame_score;
  int w_egame_score;
  int b_mgame_score;
  int b_egame_score;
};

/*
Params20241117Epoch99Step63720IntegerizedNoP: PawnStructureScores turned off
Score of Params20241117Epoch99Step63720IntegerizedNoP vs Params20241117Epoch99Step63720Integerized: 747 - 1131 - 584  [0.422] 2462
...      Params20241117Epoch99Step63720IntegerizedNoP playing White: 359 - 573 - 299  [0.413] 1231
...      Params20241117Epoch99Step63720IntegerizedNoP playing Black: 388 - 558 - 285  [0.431] 1231
...      White vs Black: 917 - 961 - 584  [0.491] 2462
Elo difference: -54.6 +/- 12.1, LOS: 0.0 %, DrawRatio: 23.7 %

Params20241117Epoch99Step63720IntegerizedOnlyDP: Only DoubledPawns enabled
Score of Params20241117Epoch99Step63720IntegerizedOnlyDP vs Params20241117Epoch99Step63720Integerized: 118 - 142 - 86  [0.465] 346
...      Params20241117Epoch99Step63720IntegerizedOnlyDP playing White: 62 - 73 - 38  [0.468] 173
...      Params20241117Epoch99Step63720IntegerizedOnlyDP playing Black: 56 - 69 - 48  [0.462] 173
...      White vs Black: 131 - 129 - 86  [0.503] 346
Elo difference: -24.1 +/- 31.8, LOS: 6.8 %, DrawRatio: 24.9 %

Params20241117Epoch99Step63720IntegerizedDPPP: DoubledPawns and PassedPawns
Score of Params20241117Epoch99Step63720IntegerizedDPPP vs Params20241117Epoch99Step63720Integerized: 228 - 261 - 169  [0.475] 658
...      Params20241117Epoch99Step63720IntegerizedDPPP playing White: 106 - 139 - 85  [0.450] 330
...      Params20241117Epoch99Step63720IntegerizedDPPP playing Black: 122 - 122 - 84  [0.500] 328
...      White vs Black: 228 - 261 - 169  [0.475] 658
Elo difference: -17.4 +/- 22.9, LOS: 6.8 %, DrawRatio: 25.7 %

Params20241117Epoch99Step63720IntegerizedControl: Basically the same thing as Params20241117Epoch99Step63720Integerized
Score of Params20241117Epoch99Step63720IntegerizedControl vs Params20241117Epoch99Step63720Integerized: 239 - 234 - 190  [0.504] 663
...      Params20241117Epoch99Step63720IntegerizedControl playing White: 118 - 121 - 93  [0.495] 332
...      Params20241117Epoch99Step63720IntegerizedControl playing Black: 121 - 113 - 97  [0.512] 331
...      White vs Black: 231 - 242 - 190  [0.492] 663
Elo difference: 2.6 +/- 22.3, LOS: 59.1 %, DrawRatio: 28.7 %

*/
template <Side side, typename ValueType>
void AddPawnStructureScores(const StdEvalParams<ValueType>& params,
                            const Board& board, ValueType& mgame_score,
                            ValueType& egame_score) {
  {
    U64 dbl_pawns = pawns::DoubledPawns<side>(board);
    while (dbl_pawns) {
      const int sq = Lsb1(dbl_pawns);
      int index = sq;
      if constexpr (side == Side::WHITE) {
        index = index ^ 56;
      }
      mgame_score += params.doubled_pawns_mgame[index];
      egame_score += params.doubled_pawns_egame[index];
      dbl_pawns ^= (1ULL << sq);
    }
  }

  {
    U64 passed_pawns = pawns::PassedPawns<side>(board);
    while (passed_pawns) {
      const int sq = Lsb1(passed_pawns);
      int index = sq;
      if constexpr (side == Side::WHITE) {
        index = index ^ 56;
      }
      mgame_score += params.passed_pawns_mgame[index];
      egame_score += params.passed_pawns_egame[index];
      passed_pawns ^= (1ULL << sq);
    }
  }

  {
    U64 isolated_pawns = pawns::IsolatedPawns<side>(board);
    while (isolated_pawns) {
      const int sq = Lsb1(isolated_pawns);
      int index = sq;
      if constexpr (side == Side::WHITE) {
        index = index ^ 56;
      }
      mgame_score += params.isolated_pawns_mgame[index];
      egame_score += params.isolated_pawns_egame[index];
      isolated_pawns ^= (1ULL << sq);
    }
  }
}

template <typename ValueType, bool enable_pawn_hashtable>
  requires(enable_pawn_hashtable == false)
void AddWBPawnStructureScores(const StdEvalParams<ValueType>& params,
                              const Board& board, ValueType& w_mgame_score,
                              ValueType& w_egame_score,
                              ValueType& b_mgame_score,
                              ValueType& b_egame_score) {
  AddPawnStructureScores<Side::WHITE>(params, board, w_mgame_score,
                                      w_egame_score);
  AddPawnStructureScores<Side::BLACK>(params, board, b_mgame_score,
                                      b_egame_score);
}

template <typename ValueType, bool enable_pawn_hashtable>
  requires(enable_pawn_hashtable == true)
void AddWBPawnStructureScores(const StdEvalParams<ValueType>& params,
                              const Board& board, ValueType& w_mgame_score,
                              ValueType& w_egame_score,
                              ValueType& b_mgame_score,
                              ValueType& b_egame_score) {
  static constexpr int PAWN_HASHTABLE_SIZE = 2048;
  static thread_local std::array<PawnStructData, PAWN_HASHTABLE_SIZE>
      pawn_hashtable;
  const U64 pawn_zkey = board.PawnZobristKey();
  const int map_index = static_cast<int>(pawn_zkey % PAWN_HASHTABLE_SIZE);
  const auto& data = pawn_hashtable[map_index];
  if (pawn_zkey == data.pawn_zobrist_key &&
      board.BitBoard(PAWN) == data.white_bb &&
      board.BitBoard(-PAWN) == data.black_bb) {
    w_mgame_score += data.w_mgame_score;
    w_egame_score += data.w_egame_score;
    b_mgame_score += data.b_mgame_score;
    b_egame_score += data.b_egame_score;
    return;
  }
  ValueType wmscore = 0, wescore = 0, bmscore = 0, bescore = 0;
  AddWBPawnStructureScores<ValueType, false>(params, board, wmscore, wescore, bmscore, bescore);
  pawn_hashtable[map_index] = PawnStructData{
      .pawn_zobrist_key = pawn_zkey,
      .white_bb = board.BitBoard(PAWN),
      .black_bb = board.BitBoard(-PAWN),
      .w_mgame_score = wmscore,
      .w_egame_score = wescore,
      .b_mgame_score = bmscore,
      .b_egame_score = bescore,
  };
  w_mgame_score += wmscore;
  w_egame_score += wescore;
  b_mgame_score += bmscore;
  b_egame_score += bescore;
}

template <Piece piece, typename ValueType>
void AddMobilityScores(const StdEvalParams<ValueType>& params,
                       const Board& board, ValueType& mgame_score,
                       ValueType& egame_score) {
  constexpr Piece piece_type = PieceType(piece);
  constexpr Side self_side = PieceSide(piece);
  const U64 occ_bb = board.BitBoard();
  const U64 self_occ_bb = board.BitBoard(self_side);
  U64 piece_bb = board.BitBoard(piece);
  while (piece_bb) {
    const int sq = Lsb1(piece_bb);
    int index = sq;
    if constexpr (PieceSide(piece) == Side::WHITE) {
      index = index ^ 56;
    }
    U64 reach = attacks::Attacks(occ_bb, sq, piece);

    ValueType mobility = ValueType(PopCount(reach & ~self_occ_bb));
    mgame_score += mobility * params.mobility_mgame[piece_type][index];
    egame_score += mobility * params.mobility_egame[piece_type][index];

    piece_bb ^= (1ULL << sq);
  }
}

template <typename ValueType, bool score_flip = true,
          bool enable_pawn_hashtable = true,
          bool include_pawn_structure_score = true>
ValueType StaticEval(const StdEvalParams<ValueType>& params, Board& board) {
  const U64 w_king = board.BitBoard(KING);
  const U64 w_queen = board.BitBoard(QUEEN);
  const U64 w_rook = board.BitBoard(ROOK);
  const U64 w_bishop = board.BitBoard(BISHOP);
  const U64 w_knight = board.BitBoard(KNIGHT);
  const U64 w_pawn = board.BitBoard(PAWN);

  int game_phase = 0;
  ValueType w_mgame_score = 0, w_egame_score = 0;

  AddPSTScores<KING>(params, w_king, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<QUEEN>(params, w_queen, game_phase, w_mgame_score,
                      w_egame_score);
  AddPSTScores<ROOK>(params, w_rook, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<BISHOP>(params, w_bishop, game_phase, w_mgame_score,
                       w_egame_score);
  AddPSTScores<KNIGHT>(params, w_knight, game_phase, w_mgame_score,
                       w_egame_score);
  AddPSTScores<PAWN>(params, w_pawn, game_phase, w_mgame_score, w_egame_score);

  AddMobilityScores<QUEEN>(params, board, w_mgame_score, w_egame_score);
  AddMobilityScores<ROOK>(params, board, w_mgame_score, w_egame_score);
  AddMobilityScores<BISHOP>(params, board, w_mgame_score, w_egame_score);
  AddMobilityScores<KNIGHT>(params, board, w_mgame_score, w_egame_score);

  const U64 b_king = board.BitBoard(-KING);
  const U64 b_queen = board.BitBoard(-QUEEN);
  const U64 b_rook = board.BitBoard(-ROOK);
  const U64 b_bishop = board.BitBoard(-BISHOP);
  const U64 b_knight = board.BitBoard(-KNIGHT);
  const U64 b_pawn = board.BitBoard(-PAWN);

  ValueType b_mgame_score = 0, b_egame_score = 0;

  AddPSTScores<-KING>(params, b_king, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-QUEEN>(params, b_queen, game_phase, b_mgame_score,
                       b_egame_score);
  AddPSTScores<-ROOK>(params, b_rook, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-BISHOP>(params, b_bishop, game_phase, b_mgame_score,
                        b_egame_score);
  AddPSTScores<-KNIGHT>(params, b_knight, game_phase, b_mgame_score,
                        b_egame_score);
  AddPSTScores<-PAWN>(params, b_pawn, game_phase, b_mgame_score, b_egame_score);

  if constexpr (include_pawn_structure_score) {
    AddWBPawnStructureScores<ValueType, enable_pawn_hashtable>(
        params, board, w_mgame_score, w_egame_score, b_mgame_score,
        b_egame_score);
  }

  AddMobilityScores<-QUEEN>(params, board, b_mgame_score, b_egame_score);
  AddMobilityScores<-ROOK>(params, board, b_mgame_score, b_egame_score);
  AddMobilityScores<-BISHOP>(params, board, b_mgame_score, b_egame_score);
  AddMobilityScores<-KNIGHT>(params, board, b_mgame_score, b_egame_score);

  if (board.SideToMove() == Side::WHITE) {
    w_mgame_score += params.tempo_w_mgame;
    w_egame_score += params.tempo_w_egame;
  } else {
    b_mgame_score += params.tempo_b_mgame;
    b_egame_score += params.tempo_b_egame;
  }

  const int mgame_phase = std::min(24, game_phase);
  const int egame_phase = 24 - mgame_phase;
  const ValueType mgame_score = w_mgame_score - b_mgame_score;
  const ValueType egame_score = w_egame_score - b_egame_score;

  ValueType score =
      (mgame_score * mgame_phase + egame_score * egame_phase) / 24;
  if constexpr (score_flip) {
    if (board.SideToMove() == Side::BLACK) {
      score = -score;
    }
  }
  return score;
}

} // namespace standard

#endif
