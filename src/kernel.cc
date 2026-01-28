#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
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


using namespace os;
using namespace os::common;
using namespace os::drivers;
using namespace os::hardwarecommunication;
using namespace os::gui;


// NOTE: this turns GRAPHICSMODE on/off
// #define GRAPHICSMODE 
// #define NETWORK

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

void printfHex8Bytes(uint8_t key) {
  printf("0x");
  printfHex((key >> 3*8) & 0xFF); // print 3rd byte
  printfHex((key >> 2*8) & 0xFF); // print 2nd byte
  printfHex((key >> 1*8) & 0xFF); // print 1st byte
  printfHex((key >> 0*8) & 0xFF); // print 0th byte

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
    /*
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */

    InterruptManager interrupts(0x20, &gdt, &taskManager);

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
    AdvancedTechnologyAttachment ata0m(0x1F0, true);
    // printf("ATA PRIMARY MASTER: ");
    ata0m.Identify();

    AdvancedTechnologyAttachment ata0s(0x1F0, false);
    // printf("ATA PRIMARY SLAVE: ");
    ata0s.Identify();

    // char writeBuffer[512] = {0};
    // writeBuffer[0] = 'H';
    // writeBuffer[1] = 'i';
    //
    // ata0m.Write28(0,(uint8_t*)writeBuffer ,2);
    // ata0m.Flush();
    //
    // for (int i =0 ; i< 10000000; i++);
    //
    // char readBuffer[512] = {0};
    //
    // ata0m.Read28(0, (uint8_t*)readBuffer, 2);
    //
    // printf("Write To: [");
    // // printf(readBuffer);
    // printfHex(writeBuffer[0]);
    // printfHex(writeBuffer[1]);
    // printf("]\n");
    // printf("Read Back: [");
    // // printf(readBuffer);
    // printfHex(readBuffer[0]);
    // printfHex(readBuffer[1]);
    // printf("]\n");
 
    // Test 1: Write and read immediately (same boot)
    // printf("=== TEST 1: Write and immediate read ===");
    // char writeBuffer1[512] = {0};
    // writeBuffer1[0] = 'A';
    // writeBuffer1[1] = 'B';
    //
    // ata0m.Write28(0, (uint8_t*)writeBuffer1, 2);
    // ata0m.Flush();
    //
    // char readBuffer1[512] = {0};
    // ata0m.Read28(0, (uint8_t*)readBuffer1, 2);
    //
    // printf("Wrote: [");
    // printfHex(writeBuffer1[0]);
    // printfHex(writeBuffer1[1]);
    // printf("], ");
    //
    // printf("Read: [");
    // printfHex(readBuffer1[0]);
    // printfHex(readBuffer1[1]);
    // printf("]\n");

    // Test 2: Try a different sector
    // printf("=== TEST 2: Write to sector 1 ===");
    // char writeBuffer2[512] = {0};
    // writeBuffer2[0] = 'X';
    // writeBuffer2[1] = 'Y';
    //
    // ata0m.Write28(1, (uint8_t*)writeBuffer2, 2);  // Different sector
    // ata0m.Flush();
    //
    // char readBuffer2[512] = {0};
    // ata0m.Read28(1, (uint8_t*)readBuffer2, 2);
    //
    // printf("Wrote: [");
    // printfHex(writeBuffer2[0]);
    // printfHex(writeBuffer2[1]);
    // printf("], ");
    //
    // printf("Read: [");
    // printfHex(readBuffer2[0]);
    // printfHex(readBuffer2[1]);
    // printf("]\n");

    // Test 3: Read sector 0 again to see if Test 1 data is still there
    // printf("=== TEST 3: Re-read sector 0 ===");
    // char readBuffer3[512] = {0};
    // ata0m.Read28(0, (uint8_t*)readBuffer3, 2);
    //
    // printf("Read: [");
    // printfHex(readBuffer3[0]);
    // printfHex(readBuffer3[1]);
    // printf("]\n");
    // printf("=== Reading existing sector 0 ===\n");
    // char existingData[512] = {0};
    // ata0m.Read28(0, (uint8_t*)existingData, 512);  // Read full sector
    //
    // printf("First 16 bytes of sector 0: ");
    // for(int i = 0; i < 16; i++) {
    //   printfHex(existingData[i]);
    //   printf(" ");
    // }
    // printf("\n");

    // Clean test
    printf("=== CLEAN TEST ===\n");

    // Delete the old Image.img and create fresh one before running QEMU

    char buf[512] = {0};
    buf[0] = 'Z';
    buf[1] = 'Z';

    printf("Writing ZZ to sector 5...\n");
    ata0m.Write28(1, (uint8_t*)buf, 2);
    ata0m.Flush();

    char read[512] = {0};
    printf("Reading sector 5...\n");
    ata0m.Read28(1, (uint8_t*)read, 2);

    printf("Result: [");
    printfHex(read[0]);
    printfHex(read[1]);
    printf("]\n");

    printf("=== FOLLOWUP TEST ===\n");

    // Delete the old Image.img and create fresh one before running QEMU

    char buf2[512] = {0};
    buf2[0] = '2';
    buf2[1] = '3';

    printf("Writing 23 to sector 5...\n");
    ata0m.Write28(5, (uint8_t*)buf2, 2);
    ata0m.Flush();

    char read2[512] = {0};
    printf("Reading sector 5...\n");
    ata0m.Read28(5, (uint8_t*)read2, 2);

    printf("Result: [");
    printfHex(read2[0]);
    printfHex(read2[1]);
    printf("]\n");

    printf("=== FOLLOWUP TEST 2 ===\n");

    // Delete the old Image.img and create fresh one before running QEMU

    char buf3[512] = {0};
    buf3[0] = 'a';
    buf3[1] = 'b';

    printf("Writing ab to sector 5...\n");
    ata0m.Write28(5, (uint8_t*)buf3, 2);
    ata0m.Flush();

    char read3[512] = {0};
    printf("Reading sector 5...\n");
    ata0m.Read28(5, (uint8_t*)read3, 2);

    printf("Result: [");
    printfHex(read3[0]);
    printfHex(read3[1]);
    printf("]\n");

    printf("=== CLEAN TEST ===\n");

    // Delete the old Image.img and create fresh one before running QEMU

    // char buf[512] = {0};
    buf[0] = 'Z';
    buf[1] = 'Z';

    printf("Writing ZZ to sector 1...\n");
    ata0m.Write28(1, (uint8_t*)buf, 2);
    ata0m.Flush();

    // char read[512] = {0};
    printf("Reading sector 1...\n");
    ata0m.Read28(1, (uint8_t*)read, 2);

    printf("Result: [");
    printfHex(read[0]);
    printfHex(read[1]);
    printf("]\n");

    printf("=== CLEAN TEST ===\n");
    // Delete the old Image.img and create fresh one before running QEMU

    // char buf[512] = {0};
    buf[0] = 'h';
    buf[1] = 'i';

    printf("Writing hi to sector 1...\n");
    ata0m.Write28(1, (uint8_t*)buf, 2);
    ata0m.Flush();

    // char read[512] = {0};
    printf("Reading sector 1...\n");
    ata0m.Read28(1, (uint8_t*)read, 2);

    printf("Result: [");
    printfHex(read[0]);
    printfHex(read[1]);
    printf("]\n");

    printf("=== CLEAN TEST ===\n");
    // Delete the old Image.img and create fresh one before running QEMU

    // char buf[512] = {0};
    buf[0] = 'x';
    buf[1] = 'o';

    printf("Writing hi to sector 0...\n");
    ata0m.Write28(0, (uint8_t*)buf, 2);
    ata0m.Flush();

    // char read[512] = {0};
    printf("Reading sector 0...\n");
    ata0m.Read28(0, (uint8_t*)read, 2);

    printf("Result: [");
    printfHex(read[0]);
    printfHex(read[1]);
    printf("]\n");


    // secondary ATA interrupt 15, NOTE: if exists, thrid = 0x1E8, fourth = 0x168
    AdvancedTechnologyAttachment ata1m(0x170, true);
    AdvancedTechnologyAttachment ata1s(0x170, false);

#ifdef NETWORK
    amd_am79c973* eth0 = 0;
    for (int i = 0; i < drvManager.numDrivers; i++) {
      if (drvManager.drivers[i] != 0) {
        if (i == 2) {
          eth0 = (amd_am79c973*) drvManager.drivers[i];
          // printf("Found eth0 ");
          break;
        }
      }
    }

    if (eth0 != 0) {
      printf("Welcome to DracOS Network\n");

      // Create a proper 64-byte packet (minimum Ethernet frame size)
      uint8_t test_packet[64];
      for(int i = 0; i < 64; i++)
        test_packet[i] = 0;  // Zero-fill

      // Copy your message to the beginning
      char* msg = "DracOS Network Test";
      for(int i = 0; msg[i] != '\0'; i++)
        test_packet[i] = msg[i];

      eth0->Send(test_packet, 18);  // Send 64 bytes, not 18
      // printf("Packet sent\n");
    }

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

    // printf("DracOS MWHAHAHHAH !!");

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
        #ifdef GRAPHICSMODE
          desktop.Draw(&vga); 
        #endif 
    }   
}
