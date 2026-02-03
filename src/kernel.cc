#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/driver.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <common/utils.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>

// NOTE: this turns GRAPHICSMODE on/off
// #define GRAPHICSMODE 
#define NETWORK

using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;
using namespace os::gui;
using namespace os::net;




// TODO: move print functions to utils/print.h
void printf(const char* str) {
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

    /* line wrap */
    if(x >= 80){
      x = 0;
      y++;
    }

    /* scrolling */
    if (y >= 25) {
      /* copy all the lines up by 1 (row 1->0, 2->1, 3->2, ...) */
      for (int row = 0; row < 24; row++) 
        for (int col = 0; col < 80; col++) 
          VideoMemory[80*row + col] = VideoMemory[80*(row+1) + col];

      /* clear the row 24 (fill last line with spaces)*/
      for (int col = 0; col<80; col++) 
        VideoMemory[80*24 + col] = (VideoMemory[80*24 + col] & 0xFF00) | ' ';

      /* reset cursor to beginning of last line */
      x = 0;
      y = 24;
    }

    /* clear console if full, no scroll */
    /*
    if(y >= 25){
      for(y = 0; y < 25; y++)
        for(x = 0; x < 80; x++)
          VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
      x = 0;
      y = 0;
    }
    */
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

void printfHex8Bytes(uint8_t key) {
  printf("0x");
  printfHex((key >> 3*8) & 0xFF); // print 3rd byte
  printfHex((key >> 2*8) & 0xFF); // print 2nd byte
  printfHex((key >> 1*8) & 0xFF); // print 1st byte
  printfHex((key >> 0*8) & 0xFF); // print 0th byte

}

void printfHex32(uint32_t key) {
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8)  & 0xFF);
    printfHex((key)       & 0xFF);
}


// Console Event Handlers

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


// void sysprintf(char* str) {
//   asm("int $0x80" : : "a" (4), "b" (str));
// }
// multitasking test functions, 
// TODO: move to test/system/multitasking.cc
void taskA() {
  /*
  while(true) {
    printf("A");
    for (int i =0; i < 100000000; i++);
    // asm("sti");
  }
  */
  while(true){
    sysprintf("A");
  }
}
void taskB() {
  /*
  while(true) {
    printf("B"); 
    for (int i =0; i < 100000000; i++);
    // asm("sti");
  }
  */
  while(true) {
    sysprintf("B");
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

    // printf("\nheap start: 0x"); // 10 MiB heap start should be: 0x00A0000
    // printfHex((heapStart >> 3*8) & 0xFF); // byte 3 (MSB) => 0x00 & 0xFF = 0x00
    // printfHex((heapStart >> 2*8) & 0xFF); // byte 2       => 0xA0 & 0xFF = 0b(1010 0000) & 0b(1111 1111) = 0b(1010 0000) = 0xA0 
    // printfHex((heapStart >> 1*8 ) & 0xFF); // byte 1      => 0x00 & 0xFF = 0x00
    // printfHex((heapStart      ) & 0xFF); // byte 0 (LSB)  => 0x00 & 0xFF = 0x00
    // void* allocated = memoryManager.malloc(1024); // allocated 
    // printf("\nallocated: 0x");
    // printfHex(((size_t)allocated >> 24) & 0xFF);
    // printfHex(((size_t)allocated >> 16) & 0xFF);
    // printfHex(((size_t)allocated >> 8 ) & 0xFF);
    // printfHex(((size_t)allocated      ) & 0xFF);
    // printf("\n");

   
     // Multitasking/
    TaskManager taskManager;
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    /*
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    // printf("Initializing Hardware, Stage 1\n");
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


    // printf("Initializing Hardware, Stage 2\n");
      drvManager.ActivateAll();

    // printf("Initializing Hardware, Stage 3\n");

    hardwarecommunication::Port8Bit pic2Mask(0xA1);
    uint8_t mask = pic2Mask.Read();

    mask |= 0xC0;
    pic2Mask.Write(mask);
   
    // primary ATA, interrupt 14
    // AdvancedTechnologyAttachment ata0m(0x1F0, true);
    // if (ata0m.Identify())
    //   printf("ATA PRIMARY MASTER FOUND\n");



    
#ifdef NETWORK
    



    // identify eth0
    amd_am79c973* eth0 = 0;
    for (int i = 0; i < drvManager.numDrivers; i++) {
      if (drvManager.drivers[i] != 0) {
        if (i == 2) {
          eth0 = (amd_am79c973*) drvManager.drivers[i];
          printf("Found eth0 ");
          break;
        }
      }
    }

    if (eth0 != 0) 
      printf("Welcome to DracOS Network\n");


    /* IP address */
    uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
    uint32_t ip_BE   = ((uint32_t)ip4   << 24)
                      |((uint32_t)ip3   << 16)
                      |((uint32_t)ip2   <<  8)
                      |((uint32_t)ip1        );

    /* GatewayIP address  */
    uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
    uint32_t gip_BE  = ((uint32_t)gip4  << 24)
                      |((uint32_t)gip3  << 16)
                      |((uint32_t)gip2  <<  8)
                      |((uint32_t)gip1       );

    /* Subnet Mask */
    uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
    uint32_t subnet_BE  = ((uint32_t)subnet4  << 24)
                         |((uint32_t)subnet3  << 16)
                         |((uint32_t)subnet2  <<  8)
                         |((uint32_t)subnet1       );

    eth0->SetIPAddress(ip_BE); // tell network card that this is our IP

    EtherFrameProvider etherframe(eth0); // communicates with network card 

    AddressResolutionProtocol arp(&etherframe);  // communicates with etherframe provider middle-layer

    InternetProtocolProvider ipv4(&etherframe, &arp, gip_BE, subnet_BE);
#endif


#ifdef GRAPHICSMODE
    vga.SetMode(320, 200, 8); // 320x200x256
    Window win1(&desktop, 10, 10, 20, 20, 0x00, 0x00, 0xA8);
    desktop.AddChild(&win1);
    Window win2(&win1, 30, 40, 30, 30, 0x00, 0xA8, 0x00);
    desktop.AddChild(&win2);
#endif

                                                    
    // activate interupts last
    interrupts.Activate();

    // arp.Resolve(gip_BE);
    ipv4.Send(gip_BE, 0x01, (uint8_t*) "7777777",7);

    // printf("DracOS MWHAHAHHAH !!");

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
        #ifdef GRAPHICSMODE
          desktop.Draw(&vga); 
        #endif 
    }   
}
