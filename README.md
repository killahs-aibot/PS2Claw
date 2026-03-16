# PS2Claw 🕹️🤖

> ⚠️ **Experimental Alpha** — This is a proof-of-concept. Expect bugs, crashes, and weirdness. Running AI on 1999 hardware wasn't supposed to work, but here we are.

**The first AI assistant running on a 1999 PlayStation 2**

## The Mission

Run a lightweight AI assistant on original PlayStation 2 hardware. Because even a 25-year-old console deserves an agent.

## Hardware

- **CPU**: MIPS R5900 (Emotion Engine) @ ~300MHz
- **RAM**: 32MB stock (expandable to 128MB+)
- **Architecture**: MIPS III / MIPS32r2
- **Storage**: Memory card or USB

## Installation (End User)

Super simple — no Linux needed!

1. Copy `PS2CLAW.ELF` (or `PS2CLAW-packed.elf` for smaller size) to USB/memory card
2. Install FreeMCBoot: https://israpps.github.io/FreeMcBoot-Installer/
3. Launch from FreeMCBoot OSD
4. Set your API key:
   ```
   export OPENROUTER_API_KEY=your_key_here
   ```
5. Enjoy 🤖

### Optional Environment Variables

- `OPENROUTER_API_KEY` - Your OpenRouter API key (required)
- `MODEL` - Model to use (default: `google/gemini-2.0-flash-001`)

## Build from Source

### Prerequisites

```bash
# Install PS2 toolchain (prebuilt)
curl -L -o ps2dev-latest.tar.gz -LC - \
  https://github.com/ps2dev/ps2dev/releases/download/latest/ps2dev-ubuntu-latest.tar.gz
tar -xf ps2dev-latest.tar.gz -C /path/to/ps2dev

# Add to PATH
export PS2DEV=/path/to/ps2dev
export PATH=$PS2DEV/ee/bin:$PATH
```

### Build

```bash
cd ps2claw
make clean && make
```

Optional: Compress with ps2-packer:
```bash
ps2-packer PS2CLAW.ELF PS2CLAW-packed.elf
```

Output sizes:
- `PS2CLAW.ELF`: 12MB (uncompressed)
- `PS2CLAW-packed.elf`: 643KB (compressed)

## The Stack

### Native PS2 (No Linux!)
- **Launch method**: FreeMCBoot memory card homebrew
- **Target**: `mips64r5900el-ps2-elf` (bare metal)
- **No OS overhead** — runs directly on PS2 hardware

### AI Runtime
- Custom C program using PS2SDK
- Calls LLM APIs over network (PS2 Network Adaptor)
- Uses libcurl + wolfSSL for HTTPS

### GUI Design
**RETRO TERMINAL** 🏚️

- Retro CRT terminal aesthetic
- Green/amber monochrome on black
- Scanlines, flicker effects
- ASCII art headers
- Hacker BBS vibe
- Pixelated terminal font

## Key References

- [FreeMCBoot Installer](https://israpps.github.io/FreeMcBoot-Installer/) - How to run homebrew
- [PS2 Dev Wiki](https://www.psdevwiki.com/ps2/Main_Page) - PS2 developer info
- [PS2SDK](https://github.com/ps2dev/ps2sdk) - Official PS2 development kit
- [ps2dev](https://github.com/ps2dev/ps2dev) - Toolchain setup

## Challenges

- [x] Set up bare-metal MIPS toolchain (mips64r5900el-ps2-elf)
- [x] Integrate PS2SDK for network access (libcurl + wolfSSL)
- [x] Build minimal CLI that calls LLM APIs
- [ ] Add post-apocalyptic CRT GUI (future enhancement)
- [x] Package as ELF for FreeMCBoot
- [ ] Test on real hardware

## Project Structure

```
ps2claw/
├── src/
│   └── main.c          # Main program (console-only)
├── Makefile            # Build configuration
├── PS2CLAW.ELF         # Built executable (12MB)
├── PS2CLAW-packed.elf  # Compressed executable (643KB)
├── README.md           # This file
└── docs/               # Documentation
```

---

*Built by Killah + Joshua — 2026*