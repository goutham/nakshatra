#!/bin/bash

FCP=
SCP=
TIME_CONTROL="3:0"
VARIANT="suicide"
MATCH_GAMES=1
SAVE_GAME_FILE="games.pgn"
DEBUG_FILE="xboard.debug"

while (( "$#" ))
do
  case "$1" in
    -fcp)
      shift
      FCP="$1"
      ;;
    -scp)
      shift
      SCP="$1"
      ;;
    -timeControl)
      shift
      TIME_CONTROL="$1"
      ;;
    -variant)
      shift
      VARIANT="$1"
      ;;
    -matchGames)
      shift
      MATCH_GAMES="$1"
      ;;
    -saveGameFile)
      shift
      SAVE_GAME_FILE="$1"
      ;;
    -debugFile)
      shift
      DEBUG_FILE="$1"
      ;;
    *)
      echo "Unknown flag encountered"
      exit 1
  esac
  shift
done

echo Playing $MATCH_GAMES $VARIANT chess games between $FCP and $SCP, each with time control $TIME_CONTROL.
echo The PGN of all games will be saved in $SAVE_GAME_FILE

#  -initString "new\nvariant suicide\nsetboard 1nbqkbnr/1p1ppppp/r7/p1p5/P1P5/6P1/1P1PPP1P/RNBQKBNR w - -\nsb\n" \

set -x
xvfb-run -a xboard \
  -noGUI \
  -fcp $FCP \
  -scp $SCP \
  -lpf start_boards.txt \
  -lpi -1 \
  -variant $VARIANT \
  -matchGames $MATCH_GAMES \
  -testClaims false \
  -timeControl $TIME_CONTROL \
  -ruleMoves 20 \
  -repeatsToDraw 3 \
  -saveGameFile $SAVE_GAME_FILE \
  -popupExitMessage false \
  -debug \
  -nameOfDebugFile $DEBUG_FILE
set +x
