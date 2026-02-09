#include "utils/print.h"
#include <cli/shell.h>

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::hardwarecommunication;
using namespace os::drivers;

Shell::Shell() {
}


Shell::~Shell() {
}


void Shell::OnKeyDown(char c) {
  if (c == '\n') { // 'Enter' is pressed
    putChar('\n'); // print new line if 'Enter'

    if (bufferIndex > 0) { // if there is data in the buffer, echo out the command received
      commandbuffer[bufferIndex] = '\0'; // signal end of command
      bufferIndex = 0; // reset buffer index

      printf(RED_COLOR, BLACK_COLOR, "Command Received: ");
      printf(LIGHT_GRAY_COLOR, BLACK_COLOR,"%s\n", commandbuffer);
    }
    

  } 
  
  else if (c == '\b') { // 'Backspace' is pressed 
    if (bufferIndex > 0) { // only process backspace if there are characters to delete
      putChar('\b'); // visually update screen, removes character from display
      bufferIndex--;
      commandbuffer[bufferIndex] = 0; // clear the data with '0'
    }
  }

  else {
    putChar(c); // display character pressed
    commandbuffer[bufferIndex] = c; // append
    bufferIndex++;
  }


}

void Shell::fillCommandBuffer(char fill_char, uint16_t length) {
  for (uint16_t i = 0; i < length; i++) {
    commandbuffer[i] = fill_char;
  }
  bufferIndex = length % sizeof(commandbuffer)/sizeof(char) + 1; 

}



void Shell::SetPCI(PeripheralComponentInterconnectController* pciController)  {
}


void Shell::PrintPrompt() {
}


