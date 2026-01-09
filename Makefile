CC		= g++
AS 		= as
LD 		= ld

CFLAGS		 = -m32 -ffreestanding -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore
ASFLAGS 	 = --32
LDFLAGS		 = -melf_i386

OBJECTS = kernel.o loader.o gdt.o

all: mykernel.bin

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

mykernel.bin: linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(OBJECTS)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

kernel-run: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin  


kernel-debug: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -serial stdio

iso: mykernel.bin grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o myos.iso iso

run-iso: iso
	qemu-system-i386 -cdrom myos.iso



