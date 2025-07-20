#include "main.h"

int main(int argc, char** argv) {
  Protocol protocol = DetectProtocol(argc, argv);
  
  if (protocol == Protocol::UCI) {
    RunUCIMode();
  } else {
    RunXBoardMode();
  }

  return 0;
}
