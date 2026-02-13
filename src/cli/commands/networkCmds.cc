#include <cli/commands/networkCmds.h>

#include "utils/print.h"

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::cli;
using namespace os::net;


Ping::Ping(InternetControlMessageProtocol* icmp) : Command("ping", "Pings an IP address"), icmp(icmp) {
  this->icmp = icmp;
};
void Ping::execute(char* args) {
  // do not check if (this->icmp) == 0, since the check will happen in the ValidateCommandDependencies later
  // checking if this->icmp == 0 is outside of the purpose of ping
  // skipping checks makes commands atomic and avoids redundant checks in other commands
  char* ip_str = strtok(args, " ");
  uint32_t targetIP_BE = 0;

  if (ip_str) {
    uint32_t ip_parts[4] = {0, 0, 0, 0};
    uint8_t part_index = 0;
    char* octet = strtok(ip_str, ".");

    while (octet != 0 && part_index < 4) {
      ip_parts[part_index] = strToInt(octet);
      part_index++;
      octet = strtok(0, ".");  // get next octet
    }

    uint32_t targetIP_BE = convertToBigEndian(ip_parts[3], ip_parts[2], ip_parts[1], ip_parts[0]);
    printf(
        LIGHT_CYAN_COLOR, BLACK_COLOR, "Pinging %d.%d.%d.%d...\n", ip_parts[3], ip_parts[2], ip_parts[1], ip_parts[0]
    );

    this->icmp->Ping(targetIP_BE);
    return;
  } else {
    printf(LIGHT_CYAN_COLOR, BLACK_COLOR, "Usage: ping <ip.address> <flags>\n");
    return;
  }
};
