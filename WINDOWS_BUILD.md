# Building rapidsnark on Windows

The upstream rapidsnark targets Linux/macOS/iOS/Android. This fork adds Windows x64 support using MSYS2/MinGW64.

## Prerequisites

1. **MSYS2** — provides MinGW64 GCC toolchain and Unix build tools:

   ```
   winget install MSYS2.MSYS2
   ```

2. **MinGW64 packages** — run from MSYS2 MinGW64 shell (`C:\msys64\mingw64.exe`):

   ```bash
   pacman -S --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make m4 diffutils tar xz curl
   ```

3. **Clone this repo:**

   ```bash
   git clone --recursive https://github.com/Nethereum/rapidsnark.git
   cd rapidsnark
   ```

## Build

All commands run in MSYS2 MinGW64 shell.

### Step 1: Build GMP

```bash
./build_gmp.sh windows
```

This downloads GMP 6.3.0, configures with `-std=gnu17` (required for GCC 14+/15+), `--disable-assembly`, and builds a static library.

### Step 2: Build rapidsnark

```bash
make windows_x86_64
```

### Output

```
package_windows_x86_64/
├── bin/
│   ├── prover.exe
│   └── verifier.exe
├── lib/
│   ├── librapidsnark.dll      # shared library (rename to rapidsnark.dll for .NET)
│   └── libgmp.a
└── include/
    ├── prover.h
    └── verifier.h
```

### One-liner from Windows cmd/PowerShell

```
C:\msys64\usr\bin\env.exe MSYSTEM=MINGW64 C:\msys64\usr\bin\bash.exe -lc "cd /path/to/rapidsnark && ./build_gmp.sh windows && make windows_x86_64"
```

## What the Windows build changes

The `windows_x86_64` Makefile target uses these CMake flags:

| Flag | Reason |
|------|--------|
| `-DUSE_ASM=NO` | NASM x86_64 assembly not supported on Windows |
| `-DUSE_OPENMP=OFF` | Not needed, simplifies build |
| `-DBUILD_TESTS=OFF` | Test binary links pthread which complicates Windows |
| `-DCMAKE_CXX_FLAGS="-Duint=unsigned -std=c++17"` | `uint` type not defined on Windows |
| `-DCMAKE_C_FLAGS="-std=gnu17 -Duint=unsigned"` | Same, plus gnu17 for GMP compat |
| `-DCMAKE_SHARED_LINKER_FLAGS="-static"` | Links libgcc/libstdc++/libwinpthread statically |
| `-G "MSYS Makefiles"` | Use MSYS make instead of MinGW make |

## Source changes for Windows portability

### fileloader.cpp/hpp

`mmap`/`munmap`/`madvise` replaced with `CreateFileMapping`/`MapViewOfFile`/`UnmapViewOfFile`, guarded by `#ifdef _WIN32`. The POSIX path is preserved.

### BSD type aliases

`u_int64_t`/`u_int32_t` replaced with standard `uint64_t`/`uint32_t` + `#include <cstdint>` in:

- `src/`: binfile_utils, zkey_utils, wtns_utils, groth16, prover
- `depends/ffiasm/c/`: fft, multiexp, pointparallelprocessor, binfile_utils, wtns_utils, zkey_utils

### POSIX includes

Removed `#include <sys/mman.h>`, `<sys/stat.h>`, `<fcntl.h>`, `<unistd.h>` from `src/binfile_utils.cpp` (file I/O handled by fileloader).

## DLL dependencies

The resulting `librapidsnark.dll` depends only on:

```
KERNEL32.dll
msvcrt.dll
```

No MinGW runtime DLLs required (libgcc, libstdc++, libwinpthread all statically linked).

## Verified

- GCC 15.2.0 (MSYS2 MinGW64)
- GMP 6.3.0 (built from source)
- `groth16_proof_size` returns 810 (correct Groth16 JSON buffer size)
- All P/Invoke exports present: `groth16_prover`, `groth16_prover_create`, `groth16_prover_destroy`, `groth16_prover_prove`, `groth16_verify`, `groth16_proof_size`, `groth16_public_size_for_zkey_buf`
