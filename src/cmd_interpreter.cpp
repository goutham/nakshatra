#include "cmd_interpreter.h"

#include <map>
#include <string>
#include <vector>

namespace cmd_interpreter {

Command Interpret(const std::string& cmd) {
  static std::map<std::string, CmdName> cmd_map = {
    {"Error", ERROR},
    {"feature", FEATURE},
    {"force", FORCE},
    {"go", GO},
    {"move", USERMOVE},
    {"mlist", MOVELIST},
    {"omlist", OMOVELIST},
    {"new", NEW},
    {"nopost", NOPOST},
    {"otim", OTIME},
    {"ping", PING},
    {"post", POST},
    {"quit", QUIT},
    {"result", RESULT},
    {"setboard", SETBOARD},
    {"sb", SHOWBOARD},
    {"time", TIME},
    {"usermove", USERMOVE},
    {"variant", VARIANT},
    {"unmake", UNMAKE}
  };
  std::vector<std::string> parts = SplitString(cmd, ' ');
  Command command;
  auto cmd_map_kv = cmd_map.find(parts[0]);
  if (cmd_map_kv == cmd_map.end()) {
    command.cmd_name = INVALID;
  } else {
    command.cmd_name = cmd_map_kv->second;
  }
  for (int i = 1; i < parts.size(); ++i) {
    command.arguments.push_back(parts[i]);
  }
  return command;
}

}  // namespace cmd_interpreter
