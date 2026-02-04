#include <utils/print.h>

using namespace os;
using namespace os::common;
using namespace os::utils;

/* static variable to track cursor position */
static uint8_t cursorCol = 0;
static uint8_t cursorRow = 0;
static uint16_t* VideoMemory = (uint16_t*)0xB8000;

/* */
