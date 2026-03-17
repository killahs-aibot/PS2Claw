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

Super simple — no Linux needed! Choose one method:

### Option A: FreeDVDBoot (No modding required!)

1. Download FreeDVDBoot ISO: https://github.com/CTurt/FreeDVDBoot
2. Set your PS2 language to English (boot without disc → Circle → System Config → Language)
3. Burn the ISO to a DVD-R (lowest speed, finalize disc)
4. Copy `PS2CLAW.ELF` to a USB drive
5. Insert DVD, wait for uLaunchELF to load
6. Navigate to USB → PS2CLAW.ELF → Press X to run

### Option B: FreeMCBoot (Memory card hack)

1. Install FreeMCBoot: https://israpps.github.io/FreeMcBoot-Installer/
2. Copy `PS2CLAW.ELF` to USB/memory card
3. Launch from FreeMCBoot OSD

### Set your API key

Create a config file at `mc0:/PS2CLAW/config.txt` on your memory card:

```
api_key=your_openrouter_key_here
model=openai/gpt-4o-mini
demo_mode=false
```

Get a free key at: https://openrouter.ai

**Config options:**
- `api_key` - Your OpenRouter API key (required for online mode)
- `model` - Model to use (default: `google/gemini-3.1-flash-lite-preview`)
- `demo_mode` - Set to `true` for offline demo responses

Enjoy 🤖

### Set Up Network

For live AI responses, you need internet:

1. **Wired Ethernet** (recommended):
   - Use a **USB Ethernet Adapter** (Sony SCPH-10181 or AX88172-based)
   - Connect Ethernet cable from PS2 to router
   - Most routers have DHCP enabled automatically

2. **WiFi** (more complex):
   - Use a compatible WiFi adapter, or
   - Use a Powerline adapter (Ethernet over home wiring)

### Optional Config Options

- `model` - Model to use (default: `google/gemini-3.1-flash-lite-preview`)
- `demo_mode` - `true` for offline demo mode, `false` for live AI

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
**POST-APOCALYPTIC TERMINAL** 🏚️

- Retro CRT terminal aesthetic
- Green/amber monochrome on black
- Scanlines, flicker effects
- ASCII art headers
- Fallout-style / hacker bbs vibe
- Pixelated terminal font

## Key References

- [FreeDVDBoot](https://github.com/CTurt/FreeDVDBoot) - Exploit DVD player to run homebrew (no mod needed!)
- [FreeMCBoot Installer](https://israpps.github.io/FreeMcBoot-Installer/) - Memory card hack
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

## Troubleshooting

**Black screen after loading:**
- Try different USB port (front vs back)
- Ensure ELF is in correct folder
- Try OpenPS2Loader instead of SMS

**Network not working:**
- Use official Sony LAN adapter for best compatibility
- Check cable is good
- Router must have DHCP (most do)

**Slow responses:**
- PS2 network is 10/100Mbps max
- AI API takes time to respond
- Be patient - it's 1999 hardware! 😅

## Project Structure

```
ps2claw/
├── src/
│   └── main.c          # Main program (console-only)
├── config.json         # Example config (rename to config.txt for PS2)
├── Makefile            # Build configuration
├── PS2CLAW.ELF         # Built executable (12MB)
├── PS2CLAW-packed.elf # Compressed executable (643KB)
├── README.md           # This file
└── docs/               # Documentation
```

**To use on PS2:** Copy `config.txt` (not .json) to `mc0:/PS2CLAW/config.txt` on your memory card.

---

*Built by Killah + Joshua — 2026*