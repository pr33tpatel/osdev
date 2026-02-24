#ifndef __OS__CLI__COMMANDREGISTRY_H
#define __OS__CLI__COMMANDREGISTRY_H

#include <ciu/officer.h>
#include <cli/command.h>
#include <cli/commands/networkCmds.h>
#include <cli/shell.h>
#include <common/types.h>
#include <memorymanagement.h>
#include <utils/ds/hashmap.h>
#include <utils/ds/map.h>
#include <utils/print.h>


namespace os {
namespace cli {


struct DependencyEntry {
  void* ptr;
  bool isValid;
};

class CommandRegistry {
 private:
  bool ValidateGroup(const char** requiredDeps, common::uint32_t count);

 public:
  utils::ds::HashMap<const char*, DependencyEntry> dependencyMap;
  CommandRegistry();
  ~CommandRegistry();

  void InjectDependency(const char* depName, void* depPtr);
  void* GetDependency(const char* depName);


  bool ValidateSystemDependencies();
  bool ValidateNetworkDependencies();
  bool ValidateProcessDependencies();
  bool ValidateFileSystemDependencies();

  bool ValidateAllDependencies();

  bool RegisterSystemCommands();
  bool RegisterNetworkCommands();
  bool RegisterProcessCommands();
  bool RegisterFileSystemCommands();

  bool RegisterAllCommands();
};
}  // namespace cli
}  // namespace os

#endif
