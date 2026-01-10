#include "types.h"
#include "gdt.h"

void clearScreen(){
  uint16_t* VideoMemory = (unsigned short*) 0xb8000;
  for (int row = 0; row < 25; row++){
    for (int col = 0; col < 80; col++){
      int index = row * 80 + col;
      VideoMemory[index] = (0x07 << 8) | ' ';
    }
  }
}

void printf(const char* str){
  uint16_t* VideoMemory = (unsigned short*) 0xb8000;
  static int x = 0;
  static int y = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    // handle \n escape sequence for new lines
    if (str[i] == '\n') {
      x++;
      y = 0;
      continue;
    }

    int index = x * 80 + y; // set the cursor

    /*
     * so, VideoMemory[index] = (0x07 << 8) | str[i] is kinda weird syntax, but its makes sense
     * so, in standard VGA text, a character cell is represented by two bytes
     * the lower byte is the ASCII code (eg. 'A', 'B', 'Z', ...)
     * the upper byte is for the attributes (color, intensity, etc.)
     *
     *
     * 0x07 (0000 0111) is the value for a light gray color on a black background
     * performing a left shift (<<8) moves the value of 0x07 8 positions to the left, making it the upper byte,
     * so 0x07 << 8 results in: 
     *     upper: (0000 0111)
     *     lower: (0000 0000)
     *
     * now, the lower byte is used to represent the ASCII character
     * 
     * VGA color palette can be seen here: https://www.fountainware.com/EXPL/vga_color_palettes.htm
     *
     */

    VideoMemory[index] = (0x04 << 8) | str[i];
    y++;
    
    // line wrap 
    if (y >= 80) {
      y = 0;
      x++;
    }
  }
}




typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors() {
  for(constructor* i = &start_ctors; i != &end_ctors; i++) {
    (*i)();
  }
}


extern "C" void kernelMain(unsigned int magicnumber, void *multiboot_structure)  {
    clearScreen();
    printf("turn your dreams into reality \nhi there\n");

    // instantiate GDT
    GlobalDescriptorTable gdt;
    printf("\nYOO, we got this GDT upp!!");


    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}
