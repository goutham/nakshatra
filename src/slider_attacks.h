#ifndef SLIDER_ATTACKS
#define SLIDER_ATTACKS

#include "common.h"

namespace slider_attacks {

U64 RookAttacks(const U64 bitboard, const int index);

U64 BishopAttacks(const U64 bitboard, const int index);

U64 QueenAttacks(const U64 bitboard, const int index);

}  // namespace slider_attacks

#endif
