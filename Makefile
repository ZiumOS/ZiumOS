# Kompiler dan linker
CXX = g++
AS = nasm
LD = ld

# Flags kompilasi
CXXFLAGS = -m32 -fno-use-cxa-atexit -fno-rtti -fno-exceptions -nostdlib -ffreestanding -fno-stack-protector -O2 -Wall -Wextra
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Objek files
OBJS = boot.o isr.o kernel.o

# Default target
all: os.iso

# Kompilasi file assembly (.asm) menjadi objek (.o)
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Kompilasi file C++ (.cpp) menjadi objek (.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link objek files menjadi kernel
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Buat ISO
os.iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o os.iso iso

# Bersihkan file sementara
clean:
	rm -rf *.o drivers/*.o kernel.bin iso os.iso

.PHONY: all clean