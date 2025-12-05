# XAE OS Build Script for Windows (PowerShell)
# Run this if you have the tools installed on Windows natively

Write-Host "XAE OS Build System" -ForegroundColor Cyan
Write-Host "===================" -ForegroundColor Cyan
Write-Host ""

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Check for required tools
$tools = @("nasm", "gcc", "ld")
$missing = @()

foreach ($tool in $tools) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        $missing += $tool
    }
}

if ($missing.Count -gt 0) {
    Write-Host "ERROR: Missing tools: $($missing -join ', ')" -ForegroundColor Red
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Yellow
    Write-Host "1. Use WSL (Recommended):"
    Write-Host "   - Open Ubuntu/WSL terminal"
    Write-Host "   - Navigate to this folder"
    Write-Host "   - Run: make"
    Write-Host ""
    Write-Host "2. Install tools on Windows:"
    Write-Host "   - NASM: https://www.nasm.us/"
    Write-Host "   - MinGW-w64: https://www.mingw-w64.org/"
    Write-Host "   - Add to PATH"
    Write-Host ""
    exit 1
}

Write-Host "Step 1: Assembling bootloader..." -ForegroundColor Green
nasm -f bin boot/boot.asm -o build/boot.bin
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Bootloader assembly failed" -ForegroundColor Red
    exit 1
}

Write-Host "Step 2: Assembling kernel entry..." -ForegroundColor Green
nasm -f elf32 kernel/entry.asm -o build/entry.o
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Kernel entry assembly failed" -ForegroundColor Red
    exit 1
}

Write-Host "Step 3: Compiling C sources..." -ForegroundColor Green
$cflags = "-m32", "-ffreestanding", "-nostdlib", "-fno-pie", "-Wall", "-Wextra", "-O2", "-I", "kernel"

# Compile all C files
$cfiles = Get-ChildItem -Recurse -Filter "*.c" -Path "kernel"
foreach ($file in $cfiles) {
    $objname = "build/$($file.BaseName).o"
    Write-Host "  Compiling $($file.Name)..."
    gcc $cflags -c $file.FullName -o $objname
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to compile $($file.Name)" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Step 4: Linking kernel..." -ForegroundColor Green
$objfiles = Get-ChildItem -Path "build" -Filter "*.o"
ld -m elf_i386 -T linker.ld -o build/kernel.bin $objfiles
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Linking failed" -ForegroundColor Red
    exit 1
}

Write-Host "Step 5: Creating OS image..." -ForegroundColor Green
# Combine bootloader and kernel
Get-Content build/boot.bin -Encoding Byte -Raw | Set-Content build/xae_os.img -Encoding Byte
Get-Content build/kernel.bin -Encoding Byte -Raw | Add-Content build/xae_os.img -Encoding Byte

Write-Host ""
Write-Host "SUCCESS! OS built successfully!" -ForegroundColor Green
Write-Host "Image: build/xae_os.img" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run (if QEMU installed):"
Write-Host "  qemu-system-i386 -drive file=build/xae_os.img,format=raw"
Write-Host ""
