#ifndef __OS__CLI__COMMAND_H
#define __OS__CLI__COMMAND_H

namespace os {
namespace cli {

class Command {
 public:
  const char* name;
  const char* help;

  Command(const char* name, const char* help) : name(name), help(help) {}

  /**
   * pure virutal function (= 0) :
   * ->  child class implements their own execution logic
   * ->  not tied down to a object
   *
   * [----------------]
   *
   * [exeuction logic of command]
   */
  virtual void execute(char* args) = 0;
};

}  // namespace cli
}  // namespace os

#endif
