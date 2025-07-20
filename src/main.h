#ifndef MAIN_H
#define MAIN_H

enum class Protocol { XBOARD, UCI };

Protocol DetectProtocol(int argc, char** argv);
void RunXBoardMode();
void RunUCIMode();

#endif