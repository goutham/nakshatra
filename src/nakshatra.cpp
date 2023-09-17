#include "common.h"
#include "executor.h"
#include "movegen.h"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  using std::cout;
  using std::endl;
  using std::string;

  cout << "# ENGINE_NAME=" << ENGINE_NAME << endl;
  cout << "# STANDARD_TRANSPOS_SIZE=" << STANDARD_TRANSPOS_SIZE << endl;
  cout << "# ANTICHESS_TRANSPOS_SIZE=" << ANTICHESS_TRANSPOS_SIZE << endl;
  cout << "# NUM_THREADS=" << NUM_THREADS << endl;

  cout << "feature usermove=1" << endl;
  cout << "feature variants=\"normal,suicide,giveaway\"" << endl;
  cout << "feature time=1" << endl;
  cout << "feature debug=1" << endl;
  cout << "feature setboard=1" << endl;
  cout << "feature ping=1" << endl;
  cout << "feature myname=\"" << ENGINE_NAME << "\"" << endl;
  cout << "feature sigint=0" << endl;
  cout << "feature sigterm=0" << endl;
  cout << "feature done=1" << endl;

  Executor executor(ENGINE_NAME);

  string cmd_string;
  while (getline(std::cin, cmd_string)) {
    std::vector<string> response;
    executor.Execute(cmd_string, &response);
    for (const string& s : response) {
      cout << s << endl;
    }
    if (executor.quit()) {
      break;
    }
  }
  if (std::cin.bad()) {
    perror("ERROR");
  } else if (std::cin.eof()) {
    std::cerr << "ERROR: EOF found" << endl;
  } else {
    if (!executor.quit()) {
      std::cerr << "ERROR: Unknown error" << endl;
    }
  }

  return 0;
}
