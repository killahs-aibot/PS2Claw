# PS2Claw Makefile - Console-Only Version (No gsKit)
# Target: mips64r5900el-ps2-elf (PS2 bare metal)

# Toolchain paths
PS2DEV = /home/killahbot/.local/ps2dev
PS2SDK = $(PS2DEV)/ps2sdk
PREFIX = $(PS2DEV)/ee/bin/mips64r5900el-ps2-elf

CC = $(PREFIX)-gcc
CXX = $(PREFIX)-g++
LD = $(PREFIX)-ld
AR = $(PREFIX)-ar
STRIP = $(PREFIX)-strip

# PS2SDK library paths
EE_LIB = $(PS2SDK)/ee/lib
PORTS_LIB = $(PS2SDK)/ports/lib

# Include paths
EE_INC = -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
PORTS_INC = -I$(PS2SDK)/ports/include

# Compiler flags
CFLAGS = -D_EE -G0 -O2 -Wall -ffreestanding
CFLAGS += $(EE_INC) $(PORTS_INC)

# Linker flags
LDFLAGS = -L$(EE_LIB) -L$(PORTS_LIB)
LDFLAGS += -lcurl -lwolfssl -lz -lnetman -lps2ip -ldebug -lcglue -lkernel -lstdc++ -lsupc++ -lm -lkbd

# Output
TARGET = PS2CLAW.ELF
OBJS = main.o

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) -T $(PS2SDK)/ee/startup/linkfile $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "=== Build Complete ==="
	@file $(TARGET)
	@ls -lh $(TARGET)

main.o: src/main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) *.o *.elf

# For development - check toolchain
check:
	@echo "Toolchain check:"
	$(CC) --version | head -1
	@echo "SDK: $(PS2SDK)"
	@echo "Libraries:"
	ls $(EE_LIB)/libcglue.a $(PORTS_LIB)/libcurl.a 2>/dev/null