void printf(const char* str){
    unsigned short* VideoMemory = (unsigned short*) 0xb8000;

    for (int i = 0; str[i] != '\0'; i++) {
        VideoMemory[i] = (VideoMemory[i] & 0xFF00) | str[i];
    }
}
extern "C" void kernelMain(unsigned int magicnumber, void *multiboot_structure)  {
    printf("turn your dreams into reality.");

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}