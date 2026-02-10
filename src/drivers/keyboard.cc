
#include <drivers/keyboard.h>

using namespace os::common;
using namespace os::utils;
using namespace os::drivers;
using namespace os::hardwarecommunication;



KeyboardEventHandler::KeyboardEventHandler() {

}

void KeyboardEventHandler::OnKeyDown(char){

}

void KeyboardEventHandler::OnKeyUp(char){

}

KeyboardDriver::KeyboardDriver(InterruptManager* manager, KeyboardEventHandler *handler)
: InterruptHandler(manager, 0x21),
dataport(0x60),
commandport(0x64)
{
  this->handler = handler;
  this->Shift = false;
}

KeyboardDriver::~KeyboardDriver()
{
}


void KeyboardDriver::Activate() {
  while(commandport.Read() & 0x1)
    dataport.Read();
  commandport.Write(0xae); // activate interrupts
  commandport.Write(0x20); // command 0x20 = read controller command byte
  uint8_t status = (dataport.Read() | 1) & ~0x10;
  commandport.Write(0x60); // command 0x60 = set controller command byte
  dataport.Write(status);
  dataport.Write(0xf4);

}


uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = dataport.Read();

    if(handler==0) return esp;

    if (key == 0x2A || key == 0x36)  // left shfit := 0x2A, right shift := 0x36 
      Shift = true;

    if (key == 0xAA || key == 0xB6) {
      Shift = false;
    }

    /* ignore break code (break codes are when the key is release) */
    if (key & 0x80) 
      return esp; // usually, code > 0x80 is a break code (a key being released, which we don't care about)
    
    char ascii = 0;

    switch(key)
    {
      case 0x02: ascii = Shift ? '!' : '1'; break;
      case 0x03: ascii = Shift ? '@' : '2'; break;
      case 0x04: ascii = Shift ? '#' : '3'; break;
      case 0x05: ascii = Shift ? '$' : '4'; break;
      case 0x06: ascii = Shift ? '%' : '5'; break;
      case 0x07: ascii = Shift ? '^' : '6'; break;
      case 0x08: ascii = Shift ? '&' : '7'; break;
      case 0x09: ascii = Shift ? '*' : '8'; break;
      case 0x0A: ascii = Shift ? '(' : '9'; break;
      case 0x0B: ascii = Shift ? ')' : '0'; break;
      case 0x0C: ascii = Shift ? '_' : '-'; break;
      case 0x0D: ascii = Shift ? '+' : '='; break;

      case 0x10: ascii = Shift ? 'Q' : 'q'; break;
      case 0x11: ascii = Shift ? 'W' : 'w'; break;
      case 0x12: ascii = Shift ? 'E' : 'e'; break;
      case 0x13: ascii = Shift ? 'R' : 'r'; break;
      case 0x14: ascii = Shift ? 'T' : 't'; break;
      case 0x15: ascii = Shift ? 'Y' : 'y'; break;
      case 0x16: ascii = Shift ? 'U' : 'u'; break;
      case 0x17: ascii = Shift ? 'I' : 'i'; break;
      case 0x18: ascii = Shift ? 'O' : 'o'; break;
      case 0x19: ascii = Shift ? 'P' : 'p'; break;
      case 0x1A: ascii = Shift ? '{' : '['; break;
      case 0x1B: ascii = Shift ? '}' : ']'; break;
      case 0x2B: ascii = Shift ? '|' : '\\'; break;

      case 0x1E: ascii = Shift ? 'A' : 'a'; break;
      case 0x1F: ascii = Shift ? 'S' : 's'; break;
      case 0x20: ascii = Shift ? 'D' : 'd'; break;
      case 0x21: ascii = Shift ? 'F' : 'f'; break;
      case 0x22: ascii = Shift ? 'G' : 'g'; break;
      case 0x23: ascii = Shift ? 'H' : 'h'; break;
      case 0x24: ascii = Shift ? 'J' : 'j'; break;
      case 0x25: ascii = Shift ? 'K' : 'k'; break;
      case 0x26: ascii = Shift ? 'L' : 'l'; break;
      case 0x27: ascii = Shift ? ':' : ';'; break;
      case 0x28: ascii = Shift ? '"' : '\''; break;
      case 0x29: ascii = Shift ? '~' : '`'; break;

      case 0x2C: ascii = Shift ? 'Z' : 'z'; break;
      case 0x2D: ascii = Shift ? 'X' : 'x'; break;
      case 0x2E: ascii = Shift ? 'C' : 'c'; break;
      case 0x2F: ascii = Shift ? 'V' : 'v'; break;
      case 0x30: ascii = Shift ? 'B' : 'b'; break;
      case 0x31: ascii = Shift ? 'N' : 'n'; break;
      case 0x32: ascii = Shift ? 'M' : 'm'; break;
      case 0x33: ascii = Shift ? '<' : ','; break;
      case 0x34: ascii = Shift ? '>' :'.'; break;
      case 0x35: ascii = Shift ? '?' :'/'; break;

      case 0x0E: ascii = '\b'; break;
      case 0x0F: ascii = '\t'; break;
      case 0x1C: ascii = '\n'; break;
      case 0x39: ascii = ' '; break;

      // arrow keys
      case 0x48: ascii = Shift ? SHIFT_ARROW_UP :  ARROW_UP;       break;
      case 0x4D: ascii = Shift ? SHIFT_ARROW_RIGHT : ARROW_RIGHT;  break;
      case 0x50: ascii = Shift ? SHIFT_ARROW_DOWN : ARROW_DOWN;    break;
      case 0x4B: ascii = Shift ? SHIFT_ARROW_LEFT : ARROW_LEFT;    break;

                 // IGNORE CASES
      case 0x2A: case 0x36: break; // left shift and right shift
      case 0x1D: break; // ctrl key

      default:
                 {
                   printf("KEYBOARD 0x");
                   printByte(key);
                   break;
                 }
    }

    if (ascii != 0) {
      handler->OnKeyDown(ascii);
    }


    return esp;
}
