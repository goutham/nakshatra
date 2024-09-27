#include "attacks.h"
#include "board.h"
#include "common.h"
#include "egtb.h"
#include "eval.h"
#include "move_array.h"
#include "move_order.h"
#include "movegen.h"
#include "pawns.h"
#include "piece_values.h"
#include "pst.h"
#include "stopwatch.h"

#include <iostream>

namespace {

constexpr int GAME_PHASE_INC[7] = {0, 0, 4, 2, 1, 1, 0};

// TODO: Discover and set these parameters.
constexpr int DOUBLED_PAWNS_MGAME = 0;
constexpr int DOUBLED_PAWNS_EGAME = 0;
constexpr int PASSED_PAWNS_MGAME = 0;
constexpr int PASSED_PAWNS_EGAME = 0;

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

template <Side side>
void AddPawnStructureScores(const Board& board, int& mgame_score,
                            int& egame_score) {
  const int num_doubled_pawns = PopCount(pawns::DoubledPawns<side>(board));
  mgame_score += num_doubled_pawns * DOUBLED_PAWNS_MGAME;
  egame_score += num_doubled_pawns * DOUBLED_PAWNS_EGAME;

  const int num_passed_pawns = PopCount(pawns::PassedPawns<side>(board));
  mgame_score += num_passed_pawns * PASSED_PAWNS_MGAME;
  egame_score += num_passed_pawns * PASSED_PAWNS_EGAME;
}

int StaticEval(Board& board) {
  const U64 w_king = board.BitBoard(KING);
  const U64 w_queen = board.BitBoard(QUEEN);
  const U64 w_rook = board.BitBoard(ROOK);
  const U64 w_bishop = board.BitBoard(BISHOP);
  const U64 w_knight = board.BitBoard(KNIGHT);
  const U64 w_pawn = board.BitBoard(PAWN);

  int game_phase = 0;
  int w_mgame_score = 0, w_egame_score = 0;

  AddPSTScores<KING>(w_king, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<QUEEN>(w_queen, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<ROOK>(w_rook, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<BISHOP>(w_bishop, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<KNIGHT>(w_knight, game_phase, w_mgame_score, w_egame_score);
  AddPSTScores<PAWN>(w_pawn, game_phase, w_mgame_score, w_egame_score);
  AddPawnStructureScores<Side::WHITE>(board, w_mgame_score, w_egame_score);

  const U64 b_king = board.BitBoard(-KING);
  const U64 b_queen = board.BitBoard(-QUEEN);
  const U64 b_rook = board.BitBoard(-ROOK);
  const U64 b_bishop = board.BitBoard(-BISHOP);
  const U64 b_knight = board.BitBoard(-KNIGHT);
  const U64 b_pawn = board.BitBoard(-PAWN);

  int b_mgame_score = 0, b_egame_score = 0;

  AddPSTScores<-KING>(b_king, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-QUEEN>(b_queen, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-ROOK>(b_rook, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-BISHOP>(b_bishop, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-KNIGHT>(b_knight, game_phase, b_mgame_score, b_egame_score);
  AddPSTScores<-PAWN>(b_pawn, game_phase, b_mgame_score, b_egame_score);
  AddPawnStructureScores<Side::BLACK>(board, b_mgame_score, b_egame_score);

  const int mgame_phase = std::min(24, game_phase);
  const int egame_phase = 24 - mgame_phase;
  const int mgame_score = w_mgame_score - b_mgame_score;
  const int egame_score = w_egame_score - b_egame_score;

  int score = (mgame_score * mgame_phase + egame_score * egame_phase) / 24;
  if (board.SideToMove() == Side::BLACK) {
    score = -score;
  }
  return score;
}

} // namespace

template <Variant variant>
  requires(IsStandard(variant))
int Evaluate(Board& board, EGTB* egtb, int alpha, int beta) {
  assert(egtb == nullptr);
  bool in_check = attacks::InCheck(board, board.SideToMove());
  if (!in_check) {
    int standing_pat = StaticEval(board);
    if (standing_pat >= beta) {
      return standing_pat;
    }
    if (standing_pat > alpha) {
      alpha = standing_pat;
    }
  }
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);
  if (move_array.size() == 0) {
    return in_check ? -WIN : DRAW;
  }
  const MoveInfoArray move_info_array =
      OrderMoves<Variant::STANDARD>(board, move_array, nullptr);
  for (size_t i = 0; i < move_info_array.size; ++i) {
    const MoveInfo& move_info = move_info_array.moves[i];
    const Move move = move_info.move;
    if (in_check || move_info.type == MoveType::SEE_GOOD_CAPTURE) {
      board.MakeMove(move);
      int score = -Evaluate<Variant::STANDARD>(board, egtb, -beta, -alpha);
      board.UnmakeLastMove();
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

template <Variant variant>
  requires(IsStandard(variant))
int EvalResult(Board& board) {
  const Side side = board.SideToMove();
  MoveArray move_array = GenerateMoves<Variant::STANDARD>(board);
  if (move_array.size() == 0) {
    const U64 attack_map = ComputeAttackMap(board, OppositeSide(side));
    if (attack_map & board.BitBoard(PieceOfSide(KING, side))) {
      return -WIN;
    } else {
      return DRAW; // stalemate
    }
  }
  for (size_t i = 0; i < move_array.size(); ++i) {
    const Move move = move_array.get(i);
    if (board.PieceAt(move.to_index()) ==
        PieceOfSide(KING, OppositeSide(side))) {
      return WIN;
    }
  }
  return UNKNOWN;
}

template <Variant variant>
  requires(IsStandard(variant))
void WriteOutParams() {
  std::cout << "piece_order = [\"nullpiece\", \"king\", \"queen\", \"rook\", "
               "\"bishop\", \"knight\", \"pawn\",]\n";

  auto write_array = [](const int* array, size_t len,
                        const std::string& table_name) {
    std::cout << "\n" << table_name << " = [\n  ";
    for (size_t i = 0; i < len; ++i) {
      std::cout << array[i] << ", ";
      if ((i + 1) % 8 == 0) {
        std::cout << "\n  ";
      }
    }
    std::cout << "\n]\n";
  };

  std::cout << "\n[standard]\n";
  std::cout << "\n[standard.mgame]\n";
  write_array(standard::PIECE_VALUES_MGAME, 7, "piece.values");
  std::cout << "\ndoubled_pawns = " << DOUBLED_PAWNS_MGAME << "\n";
  std::cout << "\npassed_pawns = " << PASSED_PAWNS_MGAME << "\n";
  std::cout << "\n[standard.mgame.pst]\n";
  write_array(standard::PST_KING_MGAME, 64, "king");
  write_array(standard::PST_QUEEN_MGAME, 64, "queen");
  write_array(standard::PST_ROOK_MGAME, 64, "rook");
  write_array(standard::PST_BISHOP_MGAME, 64, "bishop");
  write_array(standard::PST_KNIGHT_MGAME, 64, "knight");
  write_array(standard::PST_PAWN_MGAME, 64, "pawn");

  std::cout << "\n[standard.egame]\n";
  write_array(standard::PIECE_VALUES_EGAME, 7, "piece.values");
  std::cout << "\ndoubled_pawns = " << DOUBLED_PAWNS_EGAME << "\n";
  std::cout << "\npassed_pawns = " << PASSED_PAWNS_EGAME << "\n";
  std::cout << "\n[standard.egame.pst]\n";
  write_array(standard::PST_KING_EGAME, 64, "king");
  write_array(standard::PST_QUEEN_EGAME, 64, "queen");
  write_array(standard::PST_ROOK_EGAME, 64, "rook");
  write_array(standard::PST_BISHOP_EGAME, 64, "bishop");
  write_array(standard::PST_KNIGHT_EGAME, 64, "knight");
  write_array(standard::PST_PAWN_EGAME, 64, "pawn");
}

template int Evaluate<Variant::STANDARD>(Board& board, EGTB* egtb, int alpha,
                                         int beta);
template int EvalResult<Variant::STANDARD>(Board& board);
template void WriteOutParams<Variant::STANDARD>();
