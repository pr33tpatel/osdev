#ifndef __OS__CLI__COMMANDS__NETWORKCMDS_H
#define __OS__CLI__COMMANDS__NETWORKCMDS_H

#include <cli/command.h>
#include <common/types.h>
#include <net/icmp.h>
#include <utils/math.h>
#include <utils/print.h>
#include <utils/string.h>


namespace os {
namespace cli {

class Ping : public Command {
 private:
  os::net::InternetControlMessageProtocol* icmp;

 public:
  Ping(os::net::InternetControlMessageProtocol* icmp);
  void execute(char* args) override;
};

class TracerRoute {
  // ... TracerRoute declaration here
};

}  // namespace cli
}  // namespace os

#endif
