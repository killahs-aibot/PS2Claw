# PS2Claw Build Guide

## Quick Start

### 1. Install Cross-Compiler Toolchain

```bash
# On Debian/Ubuntu
sudo apt-get update
sudo apt-get install gcc-mips-linux-gnu binutils-mips-linux-gnu libc6-dev-mips-cross

# For static linking (recommended for PS2)
sudo apt-get install libcurl4-openssl-dev
```

### 2. Build the Binary

```bash
cd /path/to/ps2claw/src
make
```

This produces a MIPS binary (~50KB with dynamic linking, ~300KB+ static).

### 3. Set API Key

```bash
export OPENROUTER_API_KEY=sk-or-v1-xxxxx...
```

### 4. Test (Host Machine)

```bash
./ps2chat.sh "Hello from PS2Claw!"
```

---

## Development Options

### Option A: Shell Script (Easiest)
- `ps2chat.sh` - Works on any Linux with curl
- Pros: No compilation needed, works on PS2
- Cons: Requires bash/sh on target

### Option B: C with libcurl
- `ps2chat.c` - Full C implementation
- Pros: Small binary, no interpreter needed
- Cons: Requires libcurl on PS2 or static linking

### Option C: Rust (Smallest, Recommended)
- See: https://github.com/zeroclaw-labs/zeroclaw
- Can produce <1MB static binaries
- Need `rustup target add mips-unknown-linux-gnu`

---

## Testing without Real Hardware

### QEMU R5900 Emulator

From: https://github.com/frno7/qemu

```bash
# Build QEMU with R5900 support
git clone https://github.com/frno7/qemu
cd qemu
./configure --target-list=mips-softmmu
make

# Test binary
qemu-mips -kernel vmlinux -append "root=/dev/sda1" -hda disk.img
```

### Pre-built Images

- Gentoo Live USB: https://archive.org/download/gentoo-ps2-liveusb-alpha1

---

## Target System Requirements

### Minimum
- 32MB RAM (PS2 stock)
- USB port
- Network adapter (for API calls)

### Recommended  
- 128MB+ RAM (via expansion)
- WiFi or ethernet adapter

### Disk
- 8GB USB drive minimum (for Gentoo live USB)
- Or just enough for kernel + small rootfs

---

## Memory Budget

PS2 has only ~32MB RAM. Here's the allocation:

| Component | Memory |
|-----------|--------|
| Linux kernel | ~10MB |
| System libs | ~8MB |
| Shell/utilities | ~4MB |
| **PS2Claw** | **<5MB** |
| Buffer | ~5MB |

Total: ~32MB (at limit!)

For safety, use 128MB expansion or optimize heavily.

---

## Troubleshooting

### "mips-linux-gnu-gcc: command not found"
Install the toolchain:
```bash
sudo apt-get install gcc-mips-linux-gnu
```

### Network not working on PS2
- Check kernel config includes network drivers
- PS2 network adapter needs specific driver
- Consider WiFi adapter instead

### Out of memory
- Use static linking with musl-libc
- Strip symbols: `mips-linux-gnu-strip ps2chat`
- Use shell script version instead of full binary

---

## References

- Linux kernel: https://github.com/frno7/linux
- PS2 Dev Wiki: https://www.psdevwiki.com/ps2/Main_Page
- Gentoo PS2: https://github.com/frno7/gentoo-mipsr5900el
- R5900 QEMU: https://github.com/frno7/qemu