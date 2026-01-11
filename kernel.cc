#include "gdt.h"
#include "VGA_COLOR_PALETTE.h"
#include "interrupts.h"
#include "utils.h"





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
    printf_VGA("turn your dreams into reality \nhi there\n");

    printf_VGA("red on blue", VGA_COLOR_RED, VGA_COLOR_BLUE);

    // instantiate GDT
    GlobalDescriptorTable gdt;
    printf_VGA("\nYOO, we got this GDT upp!!");

    InterruptManager interrupts(&gdt);

    interrupts.Activate();


    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}
