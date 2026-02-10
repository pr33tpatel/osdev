#include "utils/print.h"
#include <cli/shell.h>
#include <net/arp.h>

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::hardwarecommunication;
using namespace os::drivers;
using namespace os::net;


#define ifCmd(req_cmd) if(strcmp(cmd, req_cmd) == 0) 

Shell::Shell() {
}


Shell::~Shell() {
}


void Shell::SetPCI(PeripheralComponentInterconnectController* pciController)  {
  this->pci = pciController;
}


void Shell::SetARP(AddressResolutionProtocol* arpController) {
  this->arp = arpController;
}


void Shell::OnKeyDown(char c) {
  
  // cursor navigation
  if ((uint8_t)c == ARROW_UP) {
    moveCursor(0,-1);
    return;
  }
  if ((uint8_t)c == ARROW_RIGHT) {
    moveCursor(1,0);
    cursorIndex++;
    return;
  }
  if ((uint8_t)c == ARROW_DOWN) {
    moveCursor(0,1);
    return;
  }
  if ((uint8_t)c == ARROW_LEFT) {
    moveCursor(-1,0);
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
  if ((uint8_t) c == SHIFT_ARROW_UP) {
    printf("history");
    return;
  }




  if (c == '\n') { // 'Enter' is pressed
    putChar('\n'); // print new line if 'Enter'

    if (bufferIndex > 0) { // if there is data in the buffer, echo out the command received
      commandbuffer[bufferIndex] = '\0'; // signal end of command
      bufferIndex = 0; // reset buffer index
      ExecuteCommand();
      // PrintPreviousCmd();
    }
    PrintPrompt();
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


void Shell::PrintPrompt() {
  char* prompt = "& ";
  printf(RED_COLOR, BLACK_COLOR, prompt);
}


void Shell::PrintPreviousCmd() {
  printf(RED_COLOR, BLACK_COLOR, "Command Received: ");
  printf(LIGHT_GRAY_COLOR, BLACK_COLOR,"%s\n", commandbuffer);
}


void Shell::ShellInit() {
  PrintPrompt();
}


void Shell::ExecuteCommand() {
   if(commandbuffer[0] == '\0') {
     return;
   }

   char* cmd = strtok(commandbuffer, " "); // spaces are delimiters => signals new word

   ifCmd("whoami") 
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR,"GENESIS\n");

   ifCmd("whatami")
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "MASCHINE\n");

   ifCmd("whenami") 
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "NOW\n");

   ifCmd("howami") 
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "DIVINE GRACE\n");

   ifCmd("whyami")
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "?\n");

   ifCmd("whereami") {
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "IP: "); arp->printSrcIPAddress(); printf("\n");
     printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "MAC: "); arp->printSrcMACAddress(); printf("\n");
   }

   ifCmd("clear") 
     clearScreen();

   ifCmd("lspci") 
     pci->PrintPCIDrivers();

   

     
}
