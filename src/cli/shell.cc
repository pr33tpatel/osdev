#include <cli/shell.h>


using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::hardwarecommunication;
using namespace os::drivers;
using namespace os::net;


Shell::Shell() {}


Shell::~Shell() {}


void Shell::RegisterCommand(Command* cmd) {
  if (numCommands < 65535) {
    commandRegistry[numCommands] = cmd;
    numCommands++;
  } else {
    printf(RED_COLOR, BLACK_COLOR, "Command Registry Full. Max Commands Allowed: %d\n", numCommands);
  }
}

void Shell::ExecuteCommand() {
  if (commandbuffer[0] == '\0') return;

  char* cmdName = strtok(commandbuffer, " ");
  if (cmdName == 0) return;  // user inputted only spaces
  char* args = strtok(0, " ");

  bool found = false;
  for (int i = 0; i < numCommands; i++) {
    if (strcmp(cmdName, commandRegistry[i]->name) == 0) {
      commandRegistry[i]->execute(args);
      found = true;
      break;
    }
  }

  if (!found) {
    printf(YELLOW_COLOR, BLACK_COLOR, "\"%s\" not in Command Registry\n", cmdName);
  }
}

void Shell::fillCommandBuffer(char fill_char, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    commandbuffer[i] = fill_char;
  }
  bufferIndex = length % sizeof(commandbuffer) / sizeof(char) + 1;
}


void Shell::PrintPrompt() {
  char* prompt = "& ";
  printf(RED_COLOR, BLACK_COLOR, prompt);
}


void Shell::PrintPreviousCmd() {
  printf(RED_COLOR, BLACK_COLOR, "Command Received: ");
  printf(LIGHT_GRAY_COLOR, BLACK_COLOR, "%s\n", commandbuffer);
}

void Shell::ShellInit() {
  PrintPrompt();
}

void Shell::PrintCmdFlags(char* cmd, char** flagsList) {
  int32_t num_flags = 0;
  num_flags = sizeof(flagsList) / sizeof(flagsList[0]);  // NOLINT
  // num_flags = sizeof(*flagsList)/sizeof(**flagsList);
  printf(YELLOW_COLOR, BLACK_COLOR, "total flags for %s: %d\n", cmd, num_flags);
  printf(YELLOW_COLOR, BLACK_COLOR, "%s flags: \n", cmd);
  for (int i = 0; i < num_flags; i++) {
    printf(YELLOW_COLOR, BLACK_COLOR, "[%d] %s\n", i, flagsList[i]);
  }
}

void Shell::OnKeyDown(char c) {
  // cursor navigation
  if ((uint8_t)c == ARROW_UP) {
    moveCursor(0, -1);
    return;
  }
  if ((uint8_t)c == ARROW_RIGHT) {
    moveCursor(1, 0);
    cursorIndex++;
    return;
  }
  if ((uint8_t)c == ARROW_DOWN) {
    moveCursor(0, 1);
    return;
  }
  if ((uint8_t)c == ARROW_LEFT) {
    moveCursor(-1, 0);
    cursorIndex--;
    return;
  }


  // TODO: handle writing where data already exists by shifting existing data to the right
  /*
  if (cursorIndex < bufferIndex) {
    for (int i = bufferIndex; i > cursorIndex; i--) {
      commandbuffer[i] = commandbuffer[i-1];
    }
    commandbuffer[cursorIndex] = c;
    bufferIndex++;
    cursorIndex++;

    // TODO: printRestOfLine(commandbuffer, cursorIndex);
  }
  */


  // history
  if ((uint8_t)c == SHIFT_ARROW_UP) {
    printf("history");
    return;
  }

  if (c == '\n') {  // 'Enter' is pressed
    putChar('\n');  // print new line if 'Enter'

    if (bufferIndex > 0) {                // if there is data in the buffer, echo out the command received
      commandbuffer[bufferIndex] = '\0';  // signal end of command
      ExecuteCommand();
      bufferIndex = 0;  // reset buffer index
      // PrintPreviousCmd();
    }
    PrintPrompt();
  }

  else if (c == '\b') {     // 'Backspace' is pressed
    if (bufferIndex > 0) {  // only process backspace if there are characters to delete
      putChar('\b');        // visually update screen, removes character from display
      bufferIndex--;
      commandbuffer[bufferIndex] = 0;  // clear the data with '0'
    }
  }

  else {
    putChar(c);                      // display character pressed
    commandbuffer[bufferIndex] = c;  // append
    bufferIndex++;
  }
}
