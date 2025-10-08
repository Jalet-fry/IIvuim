# IceKing Project - Windows XP Compatible PCI Scanner

## Project Overview

IceKing is a Qt-based GUI application for scanning PCI devices on Windows XP, featuring Ice King themed animations and a graphical interface for viewing PCI configuration space.

### Key Features
- PCI device scanning via direct port I/O
- Ice King themed animations (drums, hugs, power, lost)
- Full-screen interface with device information display
- Windows XP compatible

## Technical Stack

- **Qt Version**: 5.5.1 (last version fully supporting Windows XP)
- **Compiler**: MSVC 2010 (32-bit)
- **Platform**: Windows XP and later
- **Driver**: HexIO.sys for I/O port access
- **Architecture**: x86 (32-bit) - required for inline assembly

## Compiler Compatibility Issue: MinGW vs MSVC

### The Problem

The project requires `hexiosupp.lib`, which is compiled with **MSVC** (Microsoft Visual C++). This library is **NOT compatible** with **MinGW** (Minimalist GNU for Windows).

**Key Point**: `.lib` files from MSVC and MinGW use different binary formats and calling conventions and cannot be mixed!

### Why MSVC is Required

1. **hexiosupp.lib** - compiled with MSVC compiler
2. **Inline assembly** - used in `pciwindow.cpp` for PCI configuration space access
3. **Binary format** - MSVC and MinGW use incompatible object file formats

### Compiler Comparison

| Feature | MinGW | MSVC |
|---------|-------|------|
| Cost | Free | Commercial (free Community version available) |
| Base | GCC (GNU) | Microsoft |
| Library format | .a or .lib (GNU style) | .lib (COFF) |
| C Runtime | msvcrt.dll or libgcc | msvcr*.dll (e.g., msvcr100.dll) |
| Inline assembly (x64) | Limited | Not supported |
| Inline assembly (x86) | Supported | Supported |
| Windows lib compatibility | Limited | Full |

## Building the Project

### Prerequisites

1. **Visual Studio 2010** (any edition: Express, Professional, Ultimate)
2. **Qt 5.5.1 for MSVC 2010** (32-bit)
   - Download: https://download.qt.io/archive/qt/5.5/5.5.1/
   - File: `qt-opensource-windows-x86-msvc2010-5.5.1.exe`

### Build Methods

#### Method 1: Automated Build Script (Recommended)

```cmd
# Open Visual Studio Command Prompt 2010
# Navigate to project directory
cd C:\path\to\IceKing
# Run build script
build_xp.bat
```

#### Method 2: Manual Build

```cmd
# Open Visual Studio Command Prompt 2010
cd C:\path\to\IceKing
C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe IceKing.pro
nmake release
```

#### Method 3: Qt Creator

1. Open `IceKing.pro` in Qt Creator
2. Select Kit: "Desktop Qt 5.5.1 MSVC2010 32bit"
3. Build → Build Project

## Deployment to Windows XP

### Required Files

Copy these files to a single folder on Windows XP:

```
IceKing.exe              (from release/)
HexIO.sys                (driver)
msvcp100.dll             (MSVC 2010 C++ Runtime)
msvcr100.dll             (MSVC 2010 C Runtime)
Qt5Core.dll              (from Qt installation)
Qt5Gui.dll               (from Qt installation)
Qt5Widgets.dll           (from Qt installation)
platforms/qwindows.dll   (Qt plugin)
```

### Folder Structure on XP

```
C:\IceKing\
├── IceKing.exe
├── HexIO.sys
├── msvcp100.dll
├── msvcr100.dll
├── Qt5Core.dll
├── Qt5Gui.dll
├── Qt5Widgets.dll
└── platforms\
    └── qwindows.dll
```

### Installing HexIO Driver

```cmd
# Copy driver (as Administrator)
copy HexIO.sys C:\Windows\System32\drivers\

# Install service
sc create HexIO binPath= C:\Windows\System32\drivers\HexIO.sys type= kernel start= demand

# Start service
sc start HexIO

# Check status
sc query HexIO
```

## Qt Versions for Windows XP

| Qt Version | Compilers | XP Support | Notes |
|------------|-----------|------------|-------|
| 4.8.x | MSVC 2008, 2010 | ✅ Yes | Legacy but works |
| **5.5.1** | **MSVC 2010-2013** | ✅ **Yes** | **Used in this project** |
| 5.6.x | MSVC 2010-2015 | ✅ Yes | Last LTS with XP support |
| 5.7+ | MSVC 2015+ | ❌ No | Requires Windows Vista+ |

## Troubleshooting

### "nmake: command not found"
- **Cause**: Not using Visual Studio Command Prompt
- **Solution**: Use Visual Studio Command Prompt 2010

### "unresolved external symbol"
- **Cause**: Trying to use MinGW instead of MSVC
- **Solution**: Use Qt 5.5.1 MSVC 2010 version

### "inline assembler not supported in 64-bit"
- **Cause**: Attempting 64-bit build
- **Solution**: Use 32-bit (x86) MSVC 2010 compiler

### "MSVCP100.dll not found" on XP
- **Cause**: Missing Visual C++ 2010 Runtime
- **Solution**: Copy `msvcp100.dll` and `msvcr100.dll` or install VC++ 2010 Redistributable

## Project Structure

```
IceKing/
├── main.cpp                    # Application entry point
├── mainWindow.cpp/h            # Main window with animations
├── pciwindow.cpp/h             # PCI scanning window
├── storagewindow.cpp/h         # Storage information window
├── hexioctrl.h                 # HexIO wrapper header
├── hexiosupp.lib               # HexIO library (MSVC)
├── HexIO.sys                   # I/O port access driver
├── IceKing.pro                 # Qt project file
├── img/                        # Animation frames
├── *.qrc                       # Qt resource files
└── release/
    └── IceKing.exe             # Built executable
```

## Documentation

- `COMPILER_ISSUES_GUIDE.md` - Detailed MinGW vs MSVC explanation
- `BUILD_INSTRUCTIONS_RU.md` - Complete build instructions (Russian)
- `QUICK_REFERENCE_RU.txt` - Quick reference guide (Russian)
- `README.md` - This file

## License

Educational project - check with your institution for usage terms.

## Technical Notes

### Why 32-bit Only?

The code uses inline assembly in `pciwindow.cpp`:

```cpp
__asm
{
    mov eax, configAddress
    mov dx, 0CF8h
    out dx, eax
    mov dx, 0CFCh
    in eax, dx
    mov regData, eax
}
```

**MSVC does not support inline assembly in 64-bit mode**. The project must be compiled as 32-bit (x86).

### PCI Configuration Space Access

The application reads PCI configuration space using:
1. I/O port 0xCF8 (CONFIG_ADDRESS)
2. I/O port 0xCFC (CONFIG_DATA)

This requires ring-0 access, provided by the HexIO.sys kernel driver.

## Build Verification

Check that the build is correct:

```cmd
# Check architecture (should be x86/32-bit)
dumpbin /HEADERS release\IceKing.exe | findstr "machine"
# Expected: "14C machine (x86)"

# Check dependencies
dumpbin /DEPENDENTS release\IceKing.exe
# Should include: MSVCR100.dll, MSVCP100.dll, Qt5Core.dll, etc.

# Check manifest
type release\IceKing.exe.embed.manifest | findstr "VC100"
# Should contain: Microsoft.VC100.CRT
```

## References

- Qt 5.5.1: https://download.qt.io/archive/qt/5.5/5.5.1/
- Qt Documentation: https://doc.qt.io/qt-5.5/
- Visual Studio 2010: https://visualstudio.microsoft.com/vs/older-downloads/
- PCI Configuration Space: https://wiki.osdev.org/PCI

---

**Note**: This project is specifically designed for Windows XP and uses legacy technologies. For modern systems, consider using alternative methods for PCI device enumeration.
