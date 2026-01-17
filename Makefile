CC		= g++
AS 		= as
LD 		= ld

CFLAGS		 = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin -fno-rtti -fno-exceptions -fno-leading-underscore -Wno-write-strings
ASFLAGS 	 = --32
LDFLAGS		 = -melf_i386

OBJECTS = loader.o gdt.o driver.o port.o utils.o interruptstubs.o interrupts.o keyboard.o mouse.o kernel.o

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

kernel-run-no-flicker: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -no-reboot -no-shutdown

mykernel.iso: mykernel.bin
	mkdir iso
	mkdir iso/boot
	mkdir iso/boot/grub
	cp mykernel.bin iso/boot/mykernel.bin
	echo 'set timeout=0'                      > iso/boot/grub/grub.cfg
	echo 'set default=0'                     >> iso/boot/grub/grub.cfg
	echo ''                                  >> iso/boot/grub/grub.cfg
	echo 'menuentry "My Operating System" {' >> iso/boot/grub/grub.cfg
	echo '  multiboot /boot/mykernel.bin'    >> iso/boot/grub/grub.cfg
	echo '  boot'                            >> iso/boot/grub/grub.cfg
	echo '}'                                 >> iso/boot/grub/grub.cfg
	grub-mkrescue --output=mykernel.iso iso
	rm -rf iso
kernel-run-qemu-fix: mykernel.iso
	qemu-system-i386 -cdrom mykernel.iso -boot d -m 512 -smp 1 -net none



kernel-debug: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -no-reboot -no-shutdown -serial stdio -d cpu,int

iso: mykernel.bin grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o myos.iso iso

run-iso: iso
	qemu-system-i386 -cdrom myos.iso


.PHONY: clean
clean:
	rm -r $(OBJECTS) mykernel.bin mykernel.iso
