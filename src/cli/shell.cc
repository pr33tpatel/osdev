#include <cli/shell.h>
#include <net/arp.h>

#include "utils/math.h"
#include "utils/print.h"
#include "utils/string.h"

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::hardwarecommunication;
using namespace os::drivers;
using namespace os::net;


#define ifCmd(req_cmd) if (strcmp(cmd, req_cmd) == 0)

Shell::Shell() {}


Shell::~Shell() {}


// clang-format off
void Shell::SetPCI(PeripheralComponentInterconnectController* pciController) {
  this->pci = pciController; 
}
// clang-format on


void Shell::SetNetwork(AddressResolutionProtocol* arpController, InternetControlMessageProtocol* icmpController) {
  this->arp = arpController;
  this->icmp = icmpController;
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
      bufferIndex = 0;                    // reset buffer index
      ExecuteCommand();
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


void Shell::ShellInit() { PrintPrompt(); }

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


void Shell::ExecuteCommand() {
  if (commandbuffer[0] == '\0') {
    return;
  }

  char* cmd = strtok(commandbuffer, " ");  // spaces are delimiters => signals new word

  ifCmd("whoami") printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "GENESIS\n");

  ifCmd("whatami") printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "MASCHINE\n");

  ifCmd("whenami") printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "NOW\n");

  ifCmd("howami") printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "DIVINE GRACE\n");

  ifCmd("whyami") printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "?\n");

  ifCmd("whereami") {
    printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "IP: ");
    arp->printSrcIPAddress();
    printf("\n");
    printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "MAC: ");
    arp->printSrcMACAddress();
    printf("\n");
  }

  ifCmd("clear") clearScreen();

  ifCmd("echo") {
    char* arg = strtok(0, " ");

    while (arg != 0) {
      printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s ", arg);
      arg = strtok(0, " ");
    }
    printf("\n");
  }

  ifCmd("strToInt") {
    char* arg_str = strtok(0, " ");
    if (arg_str == 0) {
      printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "Usage: strToInt <number> <base=10>\n");
    }
    char* arg_base = strtok(0, " ");

    uint16_t base = 10;
    uint32_t result = 0;

    if (arg_base != 0) {  // if the base argument is not NULL
      uint32_t parsedBase = strToInt(arg_base, 10);
      if (parsedBase != 0) {
        base = (uint16_t)parsedBase;
      }
    }

    result = strToInt(arg_str, base);
    if (base == 16) {
      printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%08x\n", result);
    } else {
      printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%d\n", result);
    }
  }

  ifCmd("convert") {
    // Usage: convert <number> <from_base> <to_base>

    char* num_str = strtok(0, " ");
    char* from_base_str = strtok(0, " ");
    char* to_base_str = strtok(0, " ");

    if (num_str && from_base_str && to_base_str) {
      uint32_t from_base = strToInt(from_base_str, 10);
      uint32_t to_base = strToInt(to_base_str, 10);
      uint32_t num = strToInt(num_str, from_base);

      char result[32];
      intToStr(num, result, to_base);

      char prefix[3] = "";
      if (to_base == 2) {
        strcpy(prefix, "0b");
      }
      if (to_base == 16) {
        strcpy(prefix, "0x");
      }


      if (to_base == 2 || to_base == 16) {
        printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s%s\n", prefix, result);
      } else
        printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s\n", result);

    } else {
      printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "Usage: convert <number> <from_base> <to_base>\n");
    }
  }

  ifCmd("ping") {
    if (this->icmp == 0) {
      printf(GREEN_COLOR, BLACK_COLOR, "ERROR: ICMP NOT INITALIZED\n");
      return;
    }
    // state variables
    char* arg = strtok(0, " ");
    uint32_t targetIP_BE = 0;
    bool ip_found = false;
    bool help_flag = false;
    bool unknown_flag = false;
    char* unknown_arg = 0;
    char* flagsList[] = {"-h", "-d", "-x", "--alert"};


    while (arg != 0) {
      if (arg[0] == '-') {  // arg is flag
        if ((strcmp(arg, "-h")) == 0) {
          help_flag = true;
        } else {
          unknown_flag = true;
          unknown_arg = arg;
          printf(YELLOW_COLOR, BLACK_COLOR, "Unknown arg: %s\n", unknown_arg);
          return;
        }
      }
      // else {
      //   // if(!ip_found) {
      //   //
      //   // }
      // }

      arg = strtok(0, " ");  // get the next arg
    }

    if (help_flag || unknown_flag) {
      printf(YELLOW_COLOR, BLACK_COLOR, "Usage: ping <target.ip> <flags>\n");
      printf(
          YELLOW_COLOR,
          BLACK_COLOR,
          "ping: Sends an echo request packet to target IP address.\nIf target receives packet, target devices sends "
          "echo reply back.\n"
      );
      PrintCmdFlags("ping", flagsList);
      return;
    }

    return;
  }

  ifCmd("pingg") {
    if (this->icmp != 0) {
      char* ip_str = strtok(0, " ");
      char* flag_str = strtok(0, " ");
      char* flag_list[] = {"-h", "-d", "-x", "--alert"};
      int32_t num_flags = sizeof(flag_list) / sizeof(flag_list[0]);
      int32_t valid_flag = 1;

      if (ip_str) {
        uint32_t ip_parts[4] = {0, 0, 0, 0};
        int8_t part_index = 0;
        char* octet = strtok(ip_str, ".");

        // FIXME: not safe as non-numeric values can be passed and will return 0 when doing strToInt (e.g. "ping abc"
        // =>, strToInt("abc") == 0 => "ping 0")
        while (octet != 0 && part_index < 4) {
          ip_parts[part_index] = strToInt(octet);
          part_index++;
          octet = strtok(0, ".");
        }

        if (flag_str) {
          valid_flag = 0;
          for (int i = 0; i < num_flags; i++) {
            if ((strcmp(flag_str, flag_list[i])) == 0) {
              valid_flag = 1;
              if (i == 0) {  // -h flag
                printf(YELLOW_COLOR, BLACK_COLOR, "Usage: ping <target.ip> <flags>\n");
                printf(
                    YELLOW_COLOR,
                    BLACK_COLOR,
                    "ping: \n\tSends an echo request packet to target IP address.\n\tIf target receives packet, target "
                    "devices sends echo reply back.\n"
                );
              }

              else if (i == 1) {
              }
            }
          }
          if (!valid_flag) {
            // prints flag options if user inputs flags and loop is exhausted
            printf(YELLOW_COLOR, BLACK_COLOR, "total flags for %s: %d\n", cmd, num_flags);
            printf(YELLOW_COLOR, BLACK_COLOR, "%s flags: \n", cmd);
            for (int i = 0; i < num_flags; i++) {
              printf(YELLOW_COLOR, BLACK_COLOR, "[%d] %s\n", i, flag_list[i]);
            }
          }
        }

        uint32_t targetIP_BE = convertToBigEndian(ip_parts[3], ip_parts[2], ip_parts[1], ip_parts[0]);

        printf(
            LIGHT_CYAN_COLOR,
            BLACK_COLOR,
            "Pinging %d.%d.%d.%d...\n",
            ip_parts[0],
            ip_parts[1],
            ip_parts[2],
            ip_parts[3]
        );

        if (valid_flag) {
          this->icmp->Ping(targetIP_BE);
        }

        return;

      } else {
        printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "Usage: ping <ip.address> <flags>\n");
        return;
      }
    } else {
      printf(GREEN_COLOR, BLACK_COLOR, "ERROR: ICMP NOT INITALIZED\n");
      return;
    }
  }


  ifCmd("lspci") { pci->PrintPCIDrivers(); }
}
