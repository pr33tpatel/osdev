#include <common/types.h>
#include <gdt.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/driver.h>
#include <common/utils.h>


using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;


void printf(const char* str)
{
  static uint16_t* VideoMemory = (uint16_t*)0xb8000;
  static uint8_t x=0,y=0;
  for(int i = 0; str[i] != '\0'; ++i) {
    switch(str[i]) {
      case '\n':
        x = 0;
        y++;
        break;
      default:
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
        x++;
        break;
    }

    if(x >= 80){
      x = 0;
      y++;
    }

    if(y >= 25){
      for(y = 0; y < 25; y++)
        for(x = 0; x < 80; x++)
          VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
      x = 0;
      y = 0;
    }
  }
}


void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler {
  public:
    void OnKeyDown(char c) {
      char* foo = " ";
      foo[0] = c;
      printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler {
  int8_t x, y;

  public:

  MouseToConsole() {
  }

  virtual void OnActivate(){
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;
    x = 40;
    y = 12;
    VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
      | ((VideoMemory[80*y+x] & 0x0F00) << 4)
      | ((VideoMemory[80*y+x] & 0x00FF));
  }

  void OnMouseMove(int8_t x_offset, int8_t y_offset){
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
      | ((VideoMemory[80*y+x] & 0x0F00) << 4)
      | ((VideoMemory[80*y+x] & 0x00FF));

    x += (int8_t)x_offset; 
    if(x<0) x = 0; //prevent mouse overflow
    if(x>=80) x = 79;

    y += (int8_t)y_offset;
    if(y<0) y = 0; //prevent mouse overflow
    if(y>=25) y = 24;

    VideoMemory[80*y+x] = ((VideoMemory[80*y+x] & 0xF000) >> 4)
      | ((VideoMemory[80*y+x] & 0x0F00) << 4)
      | ((VideoMemory[80*y+x] & 0x00FF));
  }
};

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors(){
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}


extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    // clearScreen();
    printf("Hello World! --- http://www.AlgorithMan.de\n");

    GlobalDescriptorTable gdt;
    InterruptManager interrupts(0x20, &gdt);

    printf("Initializing Hardware, Stage 1\n");
  
    DriverManager drvManager;
      
      MouseToConsole mousehandler;
      MouseDriver mouse(&interrupts, &mousehandler);
      drvManager.AddDriver(&mouse);

      PrintfKeyboardEventHandler kbhandler;
      KeyboardDriver keyboard(&interrupts, &kbhandler);
      drvManager.AddDriver(&keyboard);

      PeripheralComponentInterconnectController PCIController;
      PCIController.SelectDrivers(&drvManager);



    printf("Initializing Hardware, Stage 2\n");
      drvManager.ActivateAll();

    printf("Initializing Hardware, Stage 3\n");
    interrupts.Activate();

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}
