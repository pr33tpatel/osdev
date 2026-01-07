void printf(const char* str){
    unsigned short* VideoMemory = (unsigned short*) 0xb8000;
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
extern "C" void kernelMain(unsigned int magicnumber, void *multiboot_structure)  {
    unsigned short* vga = (unsigned short*)0xb80000;
    for (int i = 0; i < 80*25; i++) {
	vga[i] = (0x07 << 8) | ' '; 
    }
    printf("turn your dreams into reality \nhi there");

    while (1){
        asm volatile ("hlt"); // halt cpu until next interrupt, saving power and does not max out cpu usage
        // using "hlt" is better than an while(1) infinite loop because it does not waste CPU cycles, generate heat, drain battery/power, etc.
    }   
}
