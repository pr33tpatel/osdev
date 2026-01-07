CC		= g++
AS 		= as
LD 		= ld

CFLAGS		 = -m32 -ffreestanding -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASFLAGS 	 = --32
LDFLAGS		 = -melf_i386

OBJECTS = kernel.o loader.o

all: mykernel.bin

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

mykernel.bin: linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin



