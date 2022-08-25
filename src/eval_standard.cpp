#include "attacks.h"
#include "board.h"
#include "common.h"
#include "eval.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "piece_values.h"
#include "pst.h"
#include "stopwatch.h"

namespace {

constexpr int GAME_PHASE_INC[7] = {0, 0, 4, 2, 1, 1, 0};

template <Piece piece>
void AddPSTScores(U64 bb, int& game_phase, int& mgame_score, int& egame_score) {
  constexpr Piece piece_type = PieceType(piece);
  constexpr Side side = PieceSide(piece);
  constexpr const auto& pst_mgame = standard::PST_MGAME[piece_type];
  constexpr const auto& pst_egame = standard::PST_EGAME[piece_type];
  while (bb) {
    const int sq = Lsb1(bb);
    int index = sq;
    if constexpr (side == Side::WHITE) {
      index = index ^ 56;
    }
    mgame_score += pst_mgame[index] + standard::PIECE_VALUES_MGAME[piece_type];
    egame_score += pst_egame[index] + standard::PIECE_VALUES_EGAME[piece_type];
    game_phase += GAME_PHASE_INC[piece_type];
    bb ^= (1ULL << sq);
  }
}

int StaticEval(Board* board) {
  const U64 w_king = board->BitBoard(KING);
  const U64 w_queen = board->BitBoard(QUEEN);
  const U64 w_rook = board->BitBoard(ROOK);
  const U64 w_bishop = board->BitBoard(BISHOP);
  const U64 w_knight = board->BitBoard(KNIGHT);
  const U64 w_pawn = board->BitBoard(PAWN);

  int game_phase = 0;
  int w_mgame_score = 0, w_egame_score = 0;

  AddPSTScores<KING>(w_king, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<QUEEN>(w_queen, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<ROOK>(w_rook, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<BISHOP>(w_bishop, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<KNIGHT>(w_knight, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<PAWN>(w_pawn, game_phase, w_mgame_score, w_egame_score);

  const U64 b_king = board->BitBoard(-KING);
  const U64 b_queen = board->BitBoard(-QUEEN);
  const U64 b_rook = board->BitBoard(-ROOK);
  const U64 b_bishop = board->BitBoard(-BISHOP);
  const U64 b_knight = board->BitBoard(-KNIGHT);
  const U64 b_pawn = board->BitBoard(-PAWN);

  int b_mgame_score = 0, b_egame_score = 0;

  AddPSTScores<-KING>(b_king, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-QUEEN>(b_queen, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-ROOK>(b_rook, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-BISHOP>(b_bishop, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-KNIGHT>(b_knight, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-PAWN>(b_pawn, game_phase, b_mgame_score, b_egame_score);

  const int mgame_phase = std::min(24, game_phase);
  const int egame_phase = 24 - mgame_phase;
  const int mgame_score = w_mgame_score - b_mgame_score;
  const int egame_score = w_egame_score - b_egame_score;

  int score = (mgame_score * mgame_phase + egame_score * egame_phase) / 24;
  if (board->SideToMove() == Side::BLACK) {
    score = -score;
  }
  return score;
}

} // namespace

template <>
int Evaluate<Variant::STANDARD>(Board* board, int alpha, int beta) {
  bool in_check = attacks::InCheck(*board, board->SideToMove());
  if (!in_check) {
    int standing_pat = StaticEval(board);
    if (standing_pat >= beta) {
      return standing_pat;
    }
    if (standing_pat > alpha) {
      alpha = standing_pat;
    }
  }
  MoveArray move_array;
  GenerateMoves<Variant::STANDARD>(board, &move_array);
  if (move_array.size() == 0) {
    return in_check ? -WIN : DRAW;
  }
  MoveInfoArray move_info_array;
  OrderMoves<Variant::STANDARD>(board, move_array, nullptr, &move_info_array);
  for (size_t i = 0; i < move_info_array.size; ++i) {
    const MoveInfo& move_info = move_info_array.moves[i];
    const Move move = move_info.move;
    if (in_check || move_info.type == MoveType::SEE_GOOD_CAPTURE) {
      board->MakeMove(move);
      int score = -Evaluate<Variant::STANDARD>(board, -beta, -alpha);
      board->UnmakeLastMove();
      if (score >= beta) {
        return score;
      }
      if (score > alpha) {
        alpha = score;
      }
    }
  }
  return alpha;
}

template <>
int EvalResult<Variant::STANDARD>(Board* board) {
  const Side side = board->SideToMove();
  MoveArray move_array;
  GenerateMoves<Variant::STANDARD>(board, &move_array);
  if (move_array.size() == 0) {
    const U64 attack_map = ComputeAttackMap(*board, OppositeSide(side));
    if (attack_map & board->BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW; // stalemate
    }
  }
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move move = move_array.get(i);
    if (board->PieceAt(move.to_index()) ==
        PieceOfSide(KING, OppositeSide(side))) {
      return WIN;
    }
  }
  return UNKNOWN;
}
