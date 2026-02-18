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
  for (int i = 0; i < DEP_COUNT; i++) {
    this->dependencies[i] = 0;
  }
}


void CommandRegistry::injectDependency(DependencyID id, void* dep) {
  if (id >= 0 && id < DEP_COUNT) {
    this->dependencies[id] = dep;
  }
}


// SYSTEM COMMANDS
bool CommandRegistry::ValidateSystemDependencies() {
  if (dependencies[DEP_SHELL] == 0) {
    printf(RED_COLOR, BLACK_COLOR, "[SHELL] Error: Missing Shell Dependency\n");
    return false;
  }
  if (dependencies[DEP_HEAP] == 0) {
    printf(RED_COLOR, BLACK_COLOR, "[SHELL] Error: Missing Heap Dependency\n");
    return false;
  }

  return true;
}


bool CommandRegistry::ValidateAllDependencies() {
  for (int i = 0; i < DEP_COUNT - 1; i++) {
    if (dependencies[i] == 0) {
      printf(RED_COLOR, BLACK_COLOR, "[SHELL] Missing Dependency: %d\n", );
    }
  }
}


}  // namespace cli
}  // namespace os
