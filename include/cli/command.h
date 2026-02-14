#ifndef __OS__CLI__COMMAND_H
#define __OS__CLI__COMMAND_H

#include <common/types.h>
#include <utils/print.h>
#include <utils/string.h>
namespace os {
namespace cli {


// [FlagOption: maps flag name to flag bool]
struct FlagOption {
  const char* name;
  const char* description;
  bool* value;
};

class Command {
 public:
  const char* name;
  Command(const char* name) : name(name) {}
  virtual void execute(char* args) = 0;
};

inline void ParseFlags(char* current_arg, FlagOption* flags, int flagCount) {
  for (int i = 0; i < flagCount; i++) {
    // if the current arguement matches flag name ...
    if (os::utils::strcmp(current_arg, flags[i].name) == 0) {
      // ... then set the corresponding bool for the flag to true
      *(flags[i].value) = true;
      return;  // return since the match was found
    }
  }
}

inline void PrintFlags(
    FlagOption* flags,
    int flagCount,
    enum os::utils::VGAColor fg = utils::YELLOW_COLOR,
    enum os::utils::VGAColor bg = utils::BLACK_COLOR
) {
  printf(fg, bg, "Flags:\tDescription:\n");
  for (int i = 0; i < flagCount; i++) {
    printf(fg, bg, "%s:\t%s\n", flags[i].name, flags[i].description);
  }
}

}  // namespace cli
}  // namespace os

#endif
