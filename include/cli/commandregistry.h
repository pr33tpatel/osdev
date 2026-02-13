#ifndef __OS__CLI__COMMANDREGISTRY_H
#define __OS__CLI__COMMANDREGISTRY_H

#include <cli/command.h>
#include <cli/commands/networkCmds.h>
#include <cli/shell.h>
#include <common/types.h>
#include <hardwarecommunication/pci.h>
#include <hardwarecommunication/port.h>
#include <memorymanagement.h>
#include <multitasking.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <utils/print.h>


namespace os {
namespace cli {
class CommandRegistry {
 private:
  // system command dependencies
  os::cli::Shell* shell;
  os::hardwarecommunication::PeripheralComponentInterconnectController* pci;
  os::MemoryManager* heap;

  // network command dependencies
  os::net::AddressResolutionProtocol* arp;
  os::net::InternetProtocolProvider* ipv4;
  os::net::InternetControlMessageProtocol* icmp;

  // process command dependencies
  os::TaskManager* taskManager;

  // filesystem command dependencies

 public:
  CommandRegistry();
  ~CommandRegistry();

  bool setSystemCmdDependencies(
      os::cli::Shell* shell,
      os::hardwarecommunication::PeripheralComponentInterconnectController* pci,
      os::MemoryManager* heap
  );
  bool setNetworkCmdDependencies(
      os::net::AddressResolutionProtocol* arp,
      os::net::InternetProtocolProvider* ipv4,
      os::net::InternetControlMessageProtocol* icmp
  );
  bool setProcessCmdDependencies(os::TaskManager* taskManager);
  bool setFileSystemCmdDependencies();

  bool ValidateCoreDependencies();

  void RegisterSystemCommands();
  void RegisterNetworkCommands();
  void RegisterProcessCommands();
  void RegisterFileSystemCommands();

  void RegisterAllCommands();
};
}  // namespace cli
}  // namespace os

#endif
