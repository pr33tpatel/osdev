CC		= g++
AS 		= as
LD 		= ld

CFLAGS		 = -m32 -fno-use-cxa-atexit -nostdlib -fno-builtin  -fno-rtti -fno-exceptions  -Wno-write-strings -Iinclude
							# -fno-leading-underscore
							# -fno-rtti
ASFLAGS 	 = --32
LDFLAGS		 = -melf_i386

OBJECTS = obj/loader.o \
					obj/gdt.o \
					obj/memorymanagement.o \
					obj/drivers/driver.o \
					obj/hardwarecommunication/port.o \
					obj/hardwarecommunication/interruptstubs.o \
					obj/hardwarecommunication/interrupts.o \
					obj/syscalls.o \
					obj/multitasking.o \
					obj/drivers/amd_am79c973.o \
					obj/hardwarecommunication/pci.o \
					obj/drivers/keyboard.o \
					obj/drivers/mouse.o \
					obj/drivers/vga.o \
					obj/drivers/ata.o \
					obj/gui/widget.o \
					obj/gui/window.o \
					obj/gui/desktop.o \
					obj/common/utils.o \
					obj/kernel.o \


all: mykernel.bin

obj/%.o: src/%.cc
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.s
	mkdir -p $(@D)
	$(AS) $(ASFLAGS) -c $< -o $@

mykernel.bin: src/linker.ld $(OBJECTS)
	$(LD) $(LDFLAGS) -T src/linker.ld -o $@ $(OBJECTS)

install: mykernel.bin
	sudo cp $< /boot/mykernel.bin

kernel-run: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin  

kernel-run-no-flicker: mykernel.bin
	qemu-system-i386 -kernel mykernel.bin -no-reboot -no-shutdown

kernel-run-qemu-fix: mykernel.iso
	qemu-system-i386 -cdrom mykernel.iso  -m 512 -smp 1 

kernel-run-qemu-fix-no-network: mykernel.iso
	qemu-system-i386 -cdrom mykernel.iso  -m 512 -smp 1 -net none

Image.img:
	qemu-img create -f raw Image.img 128M

kernel-run-qemu-fix-debug: mykernel.iso Image.img
	qemu-system-i386 \
		-cdrom mykernel.iso \
		-boot d \
		-m 512 \
		-smp 1 \
		-net nic,model=pcnet \
		-drive id=disk,file=Image.img,format=raw,if=ide,index=0
		# -device piix4-ide,id=piix4 -device ide-hd,drive=disk,bus=piix4.0


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
	rm -rf obj mykernel.bin mykernel.iso Image.img
clean-objects:
	rm -rf obj 
