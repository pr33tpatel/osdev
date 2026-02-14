#include <cli/commands/networkCmds.h>


using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::net;


Ping::Ping(InternetControlMessageProtocol* icmp) : Command("ping"), icmp(icmp) {
  this->icmp = icmp;
};
void Ping::execute(char* args) {
  char* argvList[255];  // support up to 255 args
  uint8_t argcVal = 0;
  argvList[0] = (char*)this->name;
  argcVal++;

  // first pass of inputs, tokenize args and append to argvList
  char* token = strtok(args, " ");
  while (token != 0 && argcVal < 255) {
    argvList[argcVal] = token;
    argcVal++;
    token = strtok(0, " ");
  }


  // char* argv = strtok(args, " ");
  // uint8_t argc = 0;

  // state variables
  char* ip_str = 0;
  bool helpFlag = false;
  bool verboseFlag = false;  // print out the packet content
  bool debugFlag = false;
  bool hexFlag = false;  // display contents in hexadecimal

  char* helpStr = "Usage: <ping> <ip.address> <flags>\n";

  FlagOption flags[] = {
      {"-h", "display help contents", &helpFlag},
      {"-v", "display contents of packets", &verboseFlag},
      {"-d", "debug mode, display user input as well as variables processed", &debugFlag},
      {"-hex", "display IP's in hexadecimal", &hexFlag}
  };
  int numFlags = sizeof(flags) / sizeof(flags[0]);

  // second pass
  for (int i = 1; i < argcVal; i++) {
    char* argv = argvList[i];
    if (argv[0] == '-') {  // arg is a flag if leading character is '-'
      ParseFlags(argv, flags, numFlags);
    } else {
      if (ip_str == 0) ip_str = argv;  // the input arg is the ip_str if it is not a flag
    }
  }

  if (debugFlag) {
    for (int i = 0; i < argcVal; i++) {
      printf(LIGHT_RED_COLOR, BLACK_COLOR, "DEBUG MODE: argv[%d]: \"%s\"\n", i, argvList[i]);
    }
    if (debugFlag) printf(LIGHT_RED_COLOR, BLACK_COLOR, "DEBUG MODE: IP String Parsed: %s\n", ip_str);
  }

  if (helpFlag) {
    printf(LIGHT_RED_COLOR, BLACK_COLOR, "%s", helpStr);
    PrintFlags(flags, numFlags);
    return;
  }

  if (ip_str != 0) {
    uint32_t ip_parts[4] = {0, 0, 0, 0};
    uint8_t part_index = 0;
    char* octet = strtok(ip_str, ".");

    while (octet != 0 && part_index < 4) {
      ip_parts[part_index] = strToInt(octet);
      if (debugFlag)
        printf(
            LIGHT_RED_COLOR,
            BLACK_COLOR,
            "DEBUG MODE: Octet [%d]: Parsed: %s, Value: %d\n",
            part_index,
            octet,
            ip_parts[part_index]
        );
      part_index++;
      octet = strtok(0, ".");  // get next octet
    }

    uint32_t targetIP_BE = convertToBigEndian(ip_parts[3], ip_parts[2], ip_parts[1], ip_parts[0]);
    if (debugFlag)
      printf(
          LIGHT_RED_COLOR,
          BLACK_COLOR,
          "DEBUG MODE: Target IP Big Endian: 0x%08x = %d\n",
          targetIP_BE,
          targetIP_BE
      );
    printf(
        LIGHT_CYAN_COLOR,
        BLACK_COLOR,
        "Pinging %d.%d.%d.%d...\n",
        ip_parts[0],
        ip_parts[1],
        ip_parts[2],
        ip_parts[3]
    );

    this->icmp->Ping(targetIP_BE);
    return;
  } else {
    printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "%s\n", helpStr);
    return;
  }
};
