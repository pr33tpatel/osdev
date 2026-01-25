#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <hardwarecommunication/pci.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/driver.h>
#include <drivers/vga.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <common/utils.h>
#include <multitasking.h>


using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;
using namespace os::gui;


// NOTE: this turns GRAPHICSMODE on/off
// #define GRAPHICSMODE 

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


void taskA() {
  while(true) {
    printf("A");
    for (int i =0; i < 100000000; i++);
    // asm("sti");
  }
}
void taskB() {
  while(true) {
    printf("B"); 
    for (int i =0; i < 100000000; i++);
    // asm("sti");
  }

}



typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors(){
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}


extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    os::common::clearScreen();
    
    printf("Hello World! :)                                                           \n");

    GlobalDescriptorTable gdt;
    
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8); // HACK: find the upper memory bound in kilobytes, based on multiboot_info struct from GNU
    size_t heapStart = 10*1024*1024; // NOTE: start of heap is at 10 MiB, address: 0x00A00000
    size_t padding = 10*1024; // NOTE: padding is 10 KiB
    size_t heapSize = (*memupper)*1024 - heapStart - padding; // NOTE: (*memupper) returns the total RAM above 1 MB, multiplying by 1024 converts total available RAM above 1 MB to bytes
    // NOTE: on a 512 MB RAM system, heapSize := 500 MB => (512 MB - ~1 MB) - 10 MiB - 10 KiB = 500 MB
    /* DIAGRAM: MEMORYDIMENSION at boot
     - MEMORYDIMENSION := { [ padding (10 KB)], [ ... ], [ heapStart    ...    heapSize ]} 

    */
    MemoryManager memoryManager(heapStart, (*memupper)*1024 - heapStart - 10*1024); 

    printf("\nheap start: 0x"); // 10 MiB heap start should be: 0x00A0000
    printfHex((heapStart >> 3*8) & 0xFF); // byte 3 (MSB) => 0x00 & 0xFF = 0x00
    printfHex((heapStart >> 2*8) & 0xFF); // byte 2       => 0xA0 & 0xFF = 0b(1010 0000) & 0b(1111 1111) = 0b(1010 0000) = 0xA0 
    printfHex((heapStart >> 1*8 ) & 0xFF); // byte 1      => 0x00 & 0xFF = 0x00
    printfHex((heapStart      ) & 0xFF); // byte 0 (LSB)  => 0x00 & 0xFF = 0x00
    void* allocated = memoryManager.malloc(1024); // allocated 
    printf("\nallocated: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8 ) & 0xFF);
    printfHex(((size_t)allocated      ) & 0xFF);
    printf("\n");

   
     // Multitasking/
    TaskManager taskManager;
    /*
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */

    InterruptManager interrupts(0x20, &gdt, &taskManager);

    printf("Initializing Hardware, Stage 1\n");
#ifdef GRAPHICSMODE 
    /* NOTE: dont comment out, THIS IS THE DESKTOP */
    Desktop desktop(320, 200, 0xA8, 0x00, 0x00);
#endif

    DriverManager drvManager;

#ifdef GRAPHICSMODE
    MouseDriver mouse(&interrupts, &desktop); // NOTE: handler: &desktop attaches the mouse to the desktop
#else
      MouseToConsole mousehandler;
      MouseDriver mouse(&interrupts, &mousehandler);
#endif
      drvManager.AddDriver(&mouse);


#ifdef GRAPHICSMODE
      KeyboardDriver keyboard(&interrupts, &desktop);
#else
      PrintfKeyboardEventHandler kbhandler;
      KeyboardDriver keyboard(&interrupts, &kbhandler);
#endif
      drvManager.AddDriver(&keyboard);

      PeripheralComponentInterconnectController PCIController;
      PCIController.SelectDrivers(&drvManager, &interrupts);

      VideoGraphicsArray vga;

    printf("Initializing Hardware, Stage 2\n");
      drvManager.ActivateAll();

    printf("Initializing Hardware, Stage 3\n");
    

#ifdef GRAPHICSMODE
    vga.SetMode(320, 200, 8); // 320x200x256
    Window win1(&desktop, 10, 10, 20, 20, 0x00, 0x00, 0xA8);
    desktop.AddChild(&win1);
    Window win2(&win1, 30, 40, 30, 30, 0x00, 0xA8, 0x00);
    desktop.AddChild(&win2);
#endif

                                                    
    // activate interupts last
    interrupts.Activate();

    // printf("DracOS MWHAHAHHAH !!");

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
        #ifdef GRAPHICSMODE
          desktop.Draw(&vga); 
        #endif 
    }   
}
