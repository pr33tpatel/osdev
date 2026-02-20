#include <cli/commandregistry.h>


using namespace os;
using namespace os::common;
using namespace os::cli;
using namespace os::utils;
using namespace os::net;
using namespace os::hardwarecommunication;

namespace os {
namespace cli {
CommandRegistry::CommandRegistry() {}

bool CommandRegistry::ValidateGroup(const char** requiredDeps, uint32_t count) {
  bool allValid = true;
  DependencyEntry entry;
  for (common::uint32_t i = 0; i < count; i++) {
    const char* depName = requiredDeps[i];
    if (!dependencyMap.Get(&depName, &entry) || !entry.isValid) {
      printf(RED_COLOR, BLACK_COLOR, "[ERROR] MISSING DEPENDENCY: %s\n", depName);
      allValid = false;
    } else {
      // comment out to not print out every dependency
      printf(LIGHT_GRAY_COLOR, BLACK_COLOR, "[SHELL] LOADED DEPENDENCY: ");
      printf(LIGHT_MAGENTA_COLOR, BLACK_COLOR, "%s\n", depName);
    }
  }
  return allValid;
}


void CommandRegistry::InjectDependency(const char* depName, void* depPtr) {
  DependencyEntry depEntry;
  depEntry.ptr = depPtr;
  depEntry.isValid = (depPtr != 0);  // if the depPtr is null, the entry is invalid

  dependencyMap.Insert(&depName, &depEntry);
}


void* CommandRegistry::GetDependency(const char* depName) {
  DependencyEntry depEntry;
  // query the hashmap based on the depName (key) to see if it returns an depEntry (value);
  if (dependencyMap.Get(&depName, &depEntry)) {
    if (depEntry.isValid) {
      return depEntry.ptr;  // dependency is found, return the pointer of the dependency
    }
  }
  return 0;  // query failed, dependency does not exist
}


bool CommandRegistry::ValidateSystemDependencies() {
  const char* systemDeps[] = {"SYS.SHELL", "SYS.PCI", "SYS.HEAP", "SYS.TERMINAL"};
  uint32_t count = sizeof(systemDeps) / sizeof(const char*);
  bool validDep = ValidateGroup(systemDeps, count);
  if (!validDep) printf(BLACK_COLOR, LIGHT_RED_COLOR, "[SHELL] SYSTEM COMMANDS UNAVAILABLE\n\n");
  if (validDep) printf(BLACK_COLOR, LIGHT_CYAN_COLOR, "[SHELL] SYSTEM COMMANDS AVAILABLE\n\n");
  return validDep;
}


bool CommandRegistry::ValidateNetworkDependencies() {
  const char* networkDeps[] = {"NET.ARP", "NET.IPV4", "NET.ICMP"};
  uint32_t count = sizeof(networkDeps) / sizeof(const char*);
  bool validDep = ValidateGroup(networkDeps, count);
  if (!validDep) printf(BLACK_COLOR, LIGHT_RED_COLOR, "[SHELL] NETWORK COMMANDS UNAVAILABLE\n\n");
  if (validDep) printf(BLACK_COLOR, LIGHT_CYAN_COLOR, "[SHELL] NETWORK COMMANDS AVAILABLE\n\n");
  return validDep;
}


bool CommandRegistry::ValidateProcessDependencies() {
  const char* processDeps[] = {"PROC.TASKMANAGER"};
  uint32_t count = sizeof(processDeps) / sizeof(const char*);
  bool validDep = ValidateGroup(processDeps, count);
  if (!validDep) printf(BLACK_COLOR, LIGHT_RED_COLOR, "[SHELL] PROCESS COMMANDS UNAVAILABLE\n\n");
  if (validDep) printf(BLACK_COLOR, LIGHT_CYAN_COLOR, "[SHELL] PROCESS COMMANDS AVAILABLE\n\n");
  return validDep;
}


bool CommandRegistry::ValidateFileSystemDependencies() {
  const char* filesystemDeps[] = {"FS.INIT"};
  uint32_t count = sizeof(filesystemDeps) / sizeof(const char*);
  bool validDep = ValidateGroup(filesystemDeps, count);
  if (!validDep) printf(BLACK_COLOR, LIGHT_RED_COLOR, "[SHELL] FILESYSTEM COMMANDS UNAVAILABLE\n\n");
  if (validDep) printf(BLACK_COLOR, LIGHT_CYAN_COLOR, "[SHELL] FILESYSTEM COMMANDS AVAILABLE\n\n");
  return validDep;
}

bool CommandRegistry::ValidateAllDependencies() {
  printf("Validating DracOS Dependencies...\n");
  bool sys = ValidateSystemDependencies();
  bool net = ValidateNetworkDependencies();
  bool proc = ValidateProcessDependencies();
  bool fs = ValidateFileSystemDependencies();

  /* NOTE: use bitwise '&' instead of logical '&&'
   * with '&&', if one of the earlier variables fails, the entire thing short-circuits,
   * with '&, all functions will run and check every dependency
   */

  return sys & net & proc & fs;
}


}  // namespace cli
}  // namespace os
