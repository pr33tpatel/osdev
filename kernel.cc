#include "types.h"
#include "gdt.h"

void printf(const char* str){
    uint16_t* VideoMemory = (unsigned short*) 0xb8000;
    static int row = 0;
    static int col = 0;
    for (int i = 0; str[i] != '\0'; i++) {
	if (str[i] == '\n') {
	    row++;
	    col = 0;
	    continue;
	}

	int index = row * 80 + col;
	
        VideoMemory[index] = (0x07 << 8) | str[i];
	col++;
	if (col >= 80) {
	    col = 0;
	    row++;
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
    printf("\nturn your dreams into reality \nhi there");


    // instantiate GDT
    GlobalDescriptorTable gdt;
    printf("\nYOO, we got this GDT upp!!");


    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}
