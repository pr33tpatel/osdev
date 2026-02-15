#ifndef __OS__DRIVERS__TERMINAL_H
#define __OS__DRIVERS__TERMINAL_H

#include <common/types.h>
#include <utils/print.h>

namespace os {
namespace drivers {

class Terminal {
 public:
  static const common::uint16_t VGA_WIDTH = 80;
  static const common::uint16_t VGA_HEIGHT = 25;
  static const common::uint16_t HISTORY_SIZE = 800;  // store 800 lines (32 pages) of terminal history

  static Terminal* activeTerminal;

 private:
  common::uint16_t buffer[HISTORY_SIZE][VGA_WIDTH];

  common::uint16_t cursorX;
  common::uint16_t cursorY;

  common::uint16_t viewOffset;  // offset for ring buffer, determines which line is at the top of screen

  void Render();
  common::uint16_t vga_entry(char c, common::uint8_t color);

 public:
  Terminal();
  ~Terminal();

  void PutChar(char c, os::utils::VGAColor fg, os::utils::VGAColor bg);
  void PutString(const char* str);

  void ScrollUp();
  void ScrollDown();
  void ScrollToBottom();
  void Clear();
  void Backspace();
};

}  // namespace drivers
}  // namespace os

#endif
