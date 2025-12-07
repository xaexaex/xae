# XAE OS - Network Edition

## The Ultimate Filesystem-Optimized OS with Remote Access

XAE OS is a custom operating system built from scratch, designed for developers who need efficient, remote file management. Connect from anywhere, manage your files, and disconnect - everything persists on disk.

---

## 🚀 Quick Start

### Prerequisites
- **WSL/Linux** with `gcc`, `nasm`, `make`
- **QEMU** i386 emulator
- **Python 3** (for client)

### Build and Run

```bash
# Build the OS
make clean && make

# Run with networking enabled
make runnet
```

### Connect Remotely

From another terminal:

```bash
# Using the Python client (recommended)
python3 xae_client.py 10.0.0.2 23 admin admin123

# Alternative: Direct telnet to forwarded port
telnet localhost 2323
```

**Default credentials:**
- Username: `admin` / Password: `admin123`
- Username: `user` / Password: `password`

---

## 🏗️ Architecture

### Network Stack

```
┌─────────────────────────────────────────────────┐
│              Remote User                         │
│         (Python Client / Telnet)                 │
└──────────────────┬──────────────────────────────┘
                   │ TCP/IP (Port 23)
                   │ XOR Encrypted
┌──────────────────▼──────────────────────────────┐
│              QEMU Network                        │
│    (User-mode network: 10.0.0.0/24)             │
│    Port Forward: 2323 → 10.0.0.2:23             │
└──────────────────┬──────────────────────────────┘
                   │
┌──────────────────▼──────────────────────────────┐
│          XAE OS (10.0.0.2)                      │
│  ┌──────────────────────────────────────────┐  │
│  │  Network Layer                           │  │
│  │  - RTL8139 Driver                        │  │
│  │  - Minimal TCP/IP Stack                  │  │
│  │  - Session Management (5 users max)      │  │
│  └─────────────┬────────────────────────────┘  │
│                │                                 │
│  ┌─────────────▼────────────────────────────┐  │
│  │  Authentication Layer                    │  │
│  │  - User database                         │  │
│  │  - Password hashing                      │  │
│  │  - XOR encryption (key: 0x42)            │  │
│  └─────────────┬────────────────────────────┘  │
│                │                                 │
│  ┌─────────────▼────────────────────────────┐  │
│  │  XAE Shell                               │  │
│  │  - Command interpreter                   │  │
│  │  - Network output routing                │  │
│  └─────────────┬────────────────────────────┘  │
│                │                                 │
│  ┌─────────────▼────────────────────────────┐  │
│  │  XAE Filesystem (XAE-FS)                 │  │
│  │  - Priority system (LOW/NORM/HIGH/CRIT)  │  │
│  │  - Multi-tag support                     │  │
│  │  - Parent-child directories              │  │
│  │  - Auto-sync to disk                     │  │
│  └──────────────────────────────────────────┘  │
│                                                  │
│  Persistent Storage: xae_disk.img (10MB)        │
└──────────────────────────────────────────────────┘
```

---

## 🔐 Security Features

### Authentication
- User database with hashed passwords
- Simple but effective hash: `password[i] XOR (i + 0x42)`
- Multiple user support (up to 5 users)

### Encryption
- **XOR encryption** with key `0x42`
- All network traffic encrypted
- Lightweight, suitable for file operations
- **Note:** Not cryptographically secure - designed for controlled environments

### Session Management
- Maximum 5 concurrent sessions
- Each session tracked independently
- Automatic cleanup on disconnect

---

## 📡 Network Protocol

### Connection Flow

```
Client                           XAE OS
  │                                │
  │──────── TCP SYN ──────────────>│
  │<──────SYN-ACK─────────────────│
  │──────── ACK ──────────────────>│
  │                                │
  │<──── Login Prompt ────────────│
  │                                │
  │── username:password ──────────>│  (XOR encrypted)
  │                                │
  │<──── Welcome / Error ──────────│
  │                                │
  │──────── command ──────────────>│  (XOR encrypted)
  │<──────  response ──────────────│  (XOR encrypted)
  │                                │
```

### Packet Format
- **Ethernet:** RTL8139 driver handles framing
- **IP:** Source: Client, Dest: 10.0.0.2
- **TCP:** Port 23 (telnet-style)
- **Payload:** XOR encrypted with key 0x42

---

## 💻 Available Commands

Once connected, you have access to all XAE Shell commands:

### File Operations
- `mk <file>` - Create file
- `mk <dir>/` - Create directory
- `rm <file>` - Delete file
- `edit <file>` - Text editor
- `fun <file>` - View file contents

### Navigation
- `ls` - List files in current directory
- `cd <dir>` - Change directory
- `cd ..` - Go up one level
- `cd /` - Go to root

### Filesystem Management
- `sync` - Manual save to disk (auto-sync enabled)
- `tag <file> <tag>` - Add tag to file
- `find <tag>` - Find files by tag
- `pri <file> <level>` - Set priority (LOW/NORM/HIGH/CRIT)
- `debug` - Show inode table

### Utility
- `clear` - Clear screen
- `help` - Show commands

---

## 🛠️ Building from Source

### File Structure
```
xae/
├── boot/
│   └── boot.asm          # Bootloader
├── kernel/
│   ├── kernel.c          # Main kernel
│   ├── entry.asm         # Kernel entry point
│   ├── drivers/
│   │   ├── vga.c         # Display driver
│   │   ├── keyboard.c    # Input driver
│   │   ├── disk.c        # ATA/IDE driver
│   │   ├── serial.c      # Serial port driver
│   │   └── rtl8139.c     # Network card driver
│   ├── net/
│   │   └── net.c         # TCP/IP stack
│   ├── auth/
│   │   └── auth.c        # Authentication system
│   ├── fs/
│   │   └── xaefs.c       # Custom filesystem
│   ├── shell/
│   │   └── shell.c       # Command interpreter
│   └── include/          # Header files
├── xae_client.py         # Python network client
├── Makefile              # Build system
└── linker.ld             # Linker script
```

### Compile Process

```bash
# 1. Clean previous builds
make clean

# 2. Build everything
make

# This will:
# - Assemble bootloader (boot.asm)
# - Assemble kernel entry (entry.asm)
# - Compile all C files
# - Link kernel
# - Create bootable image
# - Create persistent disk image
```

### Makefile Targets

- `make` - Build OS image
- `make run` - Run locally with VGA display
- `make runnet` - Run with network support
- `make runserial` - Run with serial port (old method)
- `make debug` - Run with GDB debugging
- `make clean` - Remove build files

---

## 🔧 Configuration

### Network Settings
Edit `kernel/include/net.h`:

```c
#define MY_IP_ADDR    0x0A000002  // 10.0.0.2
#define MY_MAC_ADDR   {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}
#define TELNET_PORT   23
```

### Add Users
Edit `kernel/auth/auth.c` in `auth_init()`:

```c
auth_add_user("newuser", "newpassword");
```

### Change Encryption Key
Edit `kernel/include/auth.h` and `xae_client.py`:

```c
// In auth.h / xae_client.py
#define ENCRYPTION_KEY 0x42  // Change to any value
```

---

## 🐍 Python Client Usage

### Basic Usage

```bash
# Connect with credentials
python3 xae_client.py 10.0.0.2 23 admin admin123

# Interactive mode (prompts for credentials)
python3 xae_client.py 10.0.0.2 23
```

### Client Features
- Automatic encryption/decryption
- Interactive terminal mode
- Ctrl+C to disconnect gracefully
- Raw terminal mode for best experience

### Client API

```python
from xae_client import XAEClient

# Create client
client = XAEClient("10.0.0.2", 23)

# Connect and login
client.connect()
if client.login("admin", "admin123"):
    # Send commands
    client.send_command("ls")
    response = client.receive_response()
    print(response)
    
    # Or use interactive mode
    client.interactive_session()

client.close()
```

---

## 🎯 Use Cases

### 1. Remote File Server
- Boot XAE OS on a server or VM
- Users connect remotely to manage files
- Lightweight alternative to full Linux + SSH

### 2. Educational Platform
- Learn OS development
- Study network protocols
- Understand filesystem design

### 3. Embedded Systems
- Minimal overhead for file operations
- Direct hardware access
- Fast boot times

### 4. Development Workspace
- Quick file organization
- Priority and tag-based management
- Persistent storage

---

## 🐛 Troubleshooting

### Cannot Connect

**Problem:** `Connection refused` or timeout

**Solution:**
1. Check QEMU is running with `make runnet`
2. Verify port forwarding: `netstat -an | grep 2323`
3. Try: `telnet localhost 2323` directly
4. Check firewall settings

### Authentication Fails

**Problem:** `Authentication failed!`

**Solution:**
1. Use default credentials: `admin`/`admin123`
2. Check if user exists in `auth.c`
3. Verify encryption key matches in client and OS

### No Output Visible

**Problem:** Connected but no text appears

**Solution:**
1. Check encryption key is `0x42` in both client and OS
2. Verify Python client is decrypting data
3. Try direct telnet without encryption

### Build Errors

**Problem:** Compilation fails

**Solution:**
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential nasm qemu-system-x86

# In WSL, ensure 32-bit support
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install gcc-multilib
```

---

## 📊 Performance

### Boot Time
- **~2 seconds** from power-on to ready
- **Instant** filesystem load from disk

### Network Latency
- **<10ms** command response (local network)
- **Minimal overhead** from encryption

### Resource Usage
- **Memory:** ~4MB kernel + data
- **Disk:** 10MB persistent storage
- **CPU:** Negligible when idle

### Capacity
- **Files:** Up to 256 files/directories
- **Sessions:** Maximum 5 concurrent users
- **File size:** Limited by disk space

---

## 🔮 Roadmap

### Current (v0.2)
- ✅ RTL8139 network driver
- ✅ Minimal TCP/IP stack
- ✅ XOR encryption
- ✅ Multi-user authentication
- ✅ Session management
- ✅ Python client

### Phase 1: Enhanced Networking
- [ ] ARP protocol implementation
- [ ] ICMP (ping) support
- [ ] UDP sockets
- [ ] Better TCP state machine

### Phase 2: Advanced Security
- [ ] AES encryption
- [ ] Public key authentication
- [ ] File permissions system
- [ ] Audit logging

### Phase 3: File Transfer
- [ ] Upload/download files
- [ ] Binary file support
- [ ] Compression
- [ ] Batch operations

### Phase 4: Multi-Protocol
- [ ] HTTP server for web UI
- [ ] FTP server
- [ ] Native SSH protocol
- [ ] REST API

---

## 🤝 Contributing

This is a learning project, but contributions are welcome!

### How to Contribute
1. Fork the repository
2. Create feature branch
3. Make changes
4. Test thoroughly
5. Submit pull request

### Areas for Improvement
- More robust TCP implementation
- Better error handling
- Additional network drivers
- Enhanced security
- Performance optimizations

---

## 📄 License

MIT License - See LICENSE file

---

## 👏 Acknowledgments

- **OSDev Wiki** - Invaluable resource for OS development
- **QEMU Project** - Excellent emulation platform
- **RTL8139 Datasheet** - Network card documentation

---

## 📞 Support

- **Issues:** GitHub Issues
- **Discussions:** GitHub Discussions
- **Email:** Your contact info

---

## 🎓 Learning Resources

Want to understand how this works?

### Recommended Reading
1. **OSDev.org** - Start here for OS basics
2. **Intel x86 Manual** - Processor architecture
3. **TCP/IP Illustrated** - Network protocols
4. **PCI Local Bus Specification** - Hardware communication

### Key Concepts Demonstrated
- **Bootloader:** How OS loads into memory
- **Protected Mode:** 32-bit x86 operation
- **Memory Management:** Page allocation
- **Driver Development:** VGA, keyboard, disk, network
- **Filesystem Design:** Custom XAE-FS
- **Network Stack:** TCP/IP from scratch
- **Protocol Design:** Authentication and encryption

---

**Happy remote file managing! 🚀**
