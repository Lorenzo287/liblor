# mman library for Windows
A light implementation of the mmap functions for MinGW.

The mmap-win32 library implements a wrapper for mmap functions around the memory mapping Windows API.

Modified copy of mman-win32 from [Google Code Archive](https://code.google.com/archive/p/mman-win32)  
Original can also be found [here, on Github](https://github.com/klauspost/mman-win3).

This project was adapted from the above original to address issues found when
trying to use it in mingw32 / mingw64 to add mmap capabilities to the [Micro Mac (umac) project](https://github.com/evansm7/umac). In umac, the following code block is used:

```C
        ofd = open(rom_filename, O_RDONLY);
        // ...
        struct stat sb;
        fstat(ofd, &sb);
        off_t _rom_size = sb.st_size;
        rom_base = mmap(0, _rom_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, ofd, 0);
```

i.e., open a file in read-only mode, then create a memory map for that which can be modified.

This fails in the original mman-win32 version, as Windows doesn't allow creating a read/write mapping for a file opened in read-only mode. I've added a little fix for that - might be a bit too generic, but works for my purposes.

## Installation

The easiest way, in MSYS2 (mingw32, mingw64 etc), would be to execute the usual sequence  
  `./configure --prefix=$MSYSTEM_PREFIX --libdir=$MSYSTEM_PREFIX/lib --incdir=$MSYSTEM_PREFIX/include/sys`  
  `make`  
  `make install`  
which installs the lib and header in the directories where an unmodified project would search for them.  
Plus, add `-lmman` to the linker command for any project that uses it.
