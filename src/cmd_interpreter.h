#ifndef CMD_INTERPRETER_H
#define CMD_INTERPRETER_H

#include "common.h"

#include <string>
#include <vector>

// All supported Xboard communication protocol commands.
enum CmdName {
  ERROR,
  FEATURE,
  FORCE,
  GO,
  INVALID,  // if command entered is invalid.
  MOVELIST,
  NEW,
  NOPOST,
  OMOVELIST,
  OTIME,
  PING,
  POST,
  QUIT,
  RESULT,
  SETBOARD,
  SHOWBOARD,
  TIME,
  UNMAKE,
  USERMOVE,
  VARIANT,
};

struct Command {
  CmdName cmd_name;
  std::vector<std::string> arguments;
};

Command Interpret(const std::string& cmd);

#endif
