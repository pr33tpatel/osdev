#include <cli/commands/systemCmds.h>

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::drivers;
using namespace os::hardwarecommunication;

whoami::whoami() : Command("whoami") {}
void whoami::execute(char* args) {
  printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "ORIGIN\n");
}


echo::echo() : Command("echo") {}
void echo::execute(char* args) {
  printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s\n", args);
}


clear::clear() : Command("clear") {}
void clear::execute(char* args) {
  Terminal::activeTerminal->Clear();
}
