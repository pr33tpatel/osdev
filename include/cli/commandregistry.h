#ifndef __OS__CLI__COMMANDREGISTRY_H
#define __OS__CLI__COMMANDREGISTRY_H

#include <cli/command.h>
#include <cli/commands/networkCmds.h>
#include <cli/shell.h>
#include <common/types.h>
#include <drivers/terminal.h>
#include <hardwarecommunication/pci.h>
#include <hardwarecommunication/port.h>
#include <memorymanagement.h>
#include <multitasking.h>
#include <net/arp.h>
#include <net/icmp.h>
#include <net/ipv4.h>
#include <utils/ds/map.h>
#include <utils/print.h>


namespace os {
namespace cli {

struct DependencyEntry {
  void* ptr;
  char* depName;
};

enum DependencyID {
  // SYSTEM
  DEP_SHELL,
  DEP_PCI,
  DEP_HEAP,
  DEP_TERMINAL,

  // NETWORK
  DEP_ARP,
  DEP_IPV4,
  DEP_ICMP,

  // PROCESS
  DEP_TASKMANAGER,

  // FILESYSTEM
  DEP_FILESYSTEM,

  DEP_COUNT  // [number of dependencies, sentinel value used to bounds and looping]
};


class CommandRegistry {
 private:
  os::utils::ds::Map<int, DependencyEntry, 32> dependencyMap;
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
  void* dependencies[DEP_COUNT];
  CommandRegistry();
  ~CommandRegistry();

  void injectDependency(DependencyID id, void* dep);

  template <typename T>
  T* getDependency(DependencyID id) {
    return (T*)dependencies[id];
  }

  bool ValidateSystemDependencies();
  bool ValidateNetworkDependencies();
  bool ValidateProcessDependencies();
  bool ValidateFileSystemDependencies();

  bool ValidateAllDependencies();

  bool RegisterSystemCommands();
  bool RegisterNetworkCommands();
  bool RegisterProcessCommands();
  bool RegisterFileSystemCommands();

  void RegisterAllCommands();
};
}  // namespace cli
}  // namespace os

#endif
