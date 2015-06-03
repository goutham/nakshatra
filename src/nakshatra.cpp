#include "cmd_interpreter.h"
#include "common.h"
#include "executor.h"
#include "movegen.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
  using std::cout;
  using std::endl;
  using std::string;

  const string kNakshatra = "Nakshatra";

  cout << "feature usermove=1" << endl;
  cout << "feature variants=\"normal,suicide\"" << endl;
  cout << "feature time=1" << endl;
  cout << "feature debug=1" << endl;
  cout << "feature setboard=1" << endl;
  cout << "feature myname=\"" << kNakshatra << "\"" << endl;
  cout << "feature sigint=0" << endl;
  cout << "feature sigterm=0" << endl;
  cout << "feature done=1" << endl;

  srand(time(NULL));

  Executor executor(kNakshatra);

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
