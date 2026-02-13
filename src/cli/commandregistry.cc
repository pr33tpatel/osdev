#include <cli/commandregistry.h>


using namespace os;
using namespace os::common;
using namespace os::cli;
using namespace os::utils;
using namespace os::net;
using namespace os::hardwarecommunication;

namespace os {
namespace cli {
CommandRegistry::CommandRegistry() {
  // set all dependency pointers to null at init

  this->shell = 0;
  this->pci = 0;
  this->heap = 0;

  this->arp = 0;
  this->ipv4 = 0;
  this->icmp = 0;

  this->taskManager = 0;
}


void CommandRegistry::RegisterSystemCommands() {
  if (!setSystemCmdDependencies(this->shell, this->pci, this->heap)) return;
  if (!ValidateCoreDependencies()) return;
  printf(LIGHT_GREEN_COLOR, BLACK_COLOR, "[SHELL] Registering System Commands...\n");

  // shell->RegisterCommand(new Echo());
}


void CommandRegistry::RegisterNetworkCommands() {
  if (!setNetworkCmdDependencies(this->arp, this->ipv4, this->icmp)) return;
  printf(LIGHT_GREEN_COLOR, BLACK_COLOR, "[SHELL] Registering Network Commands...\n");

  shell->RegisterCommand(new Ping(icmp));
}


void CommandRegistry::RegisterProcessCommands() {
  if (!setProcessCmdDependencies(this->taskManager)) return;
  printf(LIGHT_GREEN_COLOR, BLACK_COLOR, "[SHELL] Registering Process Commands...\n");
}


void CommandRegistry::RegisterFileSystemCommands() {}


void CommandRegistry::RegisterAllCommands() {
  printf(LIGHT_GREEN_COLOR, BLACK_COLOR, "[SHELL] Register All Commands...\n");
  RegisterSystemCommands();
  RegisterNetworkCommands();
  RegisterProcessCommands();
  RegisterFileSystemCommands();
}


bool CommandRegistry::ValidateCoreDependencies() {
  // only fail if the core deps are missing
  // core deps: shell, heap, ...
  if (!setSystemCmdDependencies(this->shell, this->pci, this->heap)) {
    if (this->shell == 0) {
      printf(RED_COLOR, BLACK_COLOR, "KERNEL PANIC: SHELL IS NULL. CANNOT REGISTER COMAMNDS\n");
    }
    if (this->pci == 0) {
      printf(RED_COLOR, BLACK_COLOR, "KERNEL PANIC: PCI IS NULL. CANNOT REGISTER COMMANDS\n");
    }
    if (this->heap == 0) {
      printf(RED_COLOR, BLACK_COLOR, "KERNEL PANIC: HEAP IS NULL. CANNOT REGISTER COMMANDS\n");
    }
    return false;
  }

  // if (!setFileSystemCmdDependencies()) {
  //   return false;
  // }

  return true;
}


bool CommandRegistry::setSystemCmdDependencies(
    Shell* shell, PeripheralComponentInterconnectController* pci, MemoryManager* heap
) {
  this->shell = shell;
  this->pci = pci;
  this->heap = heap;
  if (this->shell == 0 || this->heap == 0) {
    printf(RED_COLOR, BLACK_COLOR, "[SHELL] FAILED TO SET: SystemCmdDependencies\n");
    return false;
  }
  return true;
}


bool CommandRegistry::setNetworkCmdDependencies(
    AddressResolutionProtocol* arp, InternetProtocolProvider* ipv4, InternetControlMessageProtocol* icmp
) {
  this->arp = arp;
  this->ipv4 = ipv4;
  this->icmp = icmp;
  if (this->arp == 0 || this->ipv4 == 0 || this->icmp == 0) {
    printf(RED_COLOR, BLACK_COLOR, "[SHELL] FAILED TO SET: NetworkCmdDependencies\n");
    return false;
  }
  return true;
}


bool CommandRegistry::setProcessCmdDependencies(TaskManager* taskManager) {
  this->taskManager = taskManager;
  if (this->taskManager == 0) {
    printf(RED_COLOR, BLACK_COLOR, "[SHELL] FAILED TO SET: ProcessCmdDependencies\n");
    return false;
  }
  return true;
}


bool CommandRegistry::setFileSystemCmdDependencies() {
  return true;
}


}  // namespace cli
}  // namespace os
