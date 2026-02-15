#include <drivers/terminal.h>

#include "utils/print.h"

using namespace os;
using namespace os::common;
using namespace os::utils;
using namespace os::drivers;

Terminal* Terminal::activeTerminal = 0;

Terminal::Terminal() {
  activeTerminal = this;
  cursorX = 0;
  cursorY = 0;
  viewOffset = 0;
  Clear();
}


uint16_t Terminal::vga_entry(char c, uint8_t color) {
  return (uint16_t)c | ((uint16_t)color << 8);
}


void Terminal::PutChar(char c, VGAColor fg, VGAColor bg) {
  uint8_t color = (uint8_t)fg | ((uint8_t)bg << 4);

  if (c == '\n') {  // new line
    cursorY++;
    cursorX = 0;
  } else if (c == '\b') {  // backspace
    Backspace();
    return;
  } else if (c == '\t') {  // tabs are 4 space
    cursorX += 4;
  } else {
    buffer[cursorY][cursorX] = vga_entry(c, color);  // write to buffer
    cursorX++;
  }

  // line wrap
  if (cursorX >= VGA_WIDTH) {
    cursorY++;
    cursorX = 0;
  }

  // scroll buffer
  if (cursorY >= HISTORY_SIZE) {
    // shift view up by one line
    for (int y = 0; y < HISTORY_SIZE - 1; y++) {
      for (int x = 0; x < VGA_WIDTH; x++) {
        buffer[y][x] = buffer[y + 1][x];
      }
    }

    uint16_t blank = 0x0700 | ' ';
    for (int x = 0; x < VGA_WIDTH; x++) {
      buffer[HISTORY_SIZE - 1][x] = blank;
    }
    cursorY = HISTORY_SIZE - 1;
  }

  ScrollToBottom();  // snap to bottom so you are always typing on the bottom line
}

void Terminal::ScrollUp() {
  if (viewOffset > 0) {
    viewOffset--;
    Render();
  }
}


void Terminal::ScrollDown() {
  // check to make sure you don't scroll past bottom of buffer contents
  if (viewOffset + VGA_HEIGHT < cursorY + 1) {
    // allow scrolling up to last line of buffer contents
    viewOffset++;
    Render();
  }
}


void Terminal::ScrollToBottom() {
  if (cursorY < VGA_HEIGHT) {
    viewOffset = 0;
  } else {
    viewOffset = cursorY - VGA_HEIGHT + 1;
  }
  Render();
}


void Terminal::Clear() {}
