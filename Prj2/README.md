# Project 2: Inter-Process Communication and Synchronization

## Submitted Files

1. `Makefile` - A build script that compiles all C source files into executables.
2. `prj2_report.pdf` - A detailed report explaining the project.
3. **StoogeFarmers/**
   - `lcm.c` - Implements the core function in the Stooge Farmer Problem.
   - `lcm_main.c` - The program entrance.
   - `lcm.h` - The header.
   - `Makefile` - A Makefile script for StoogeFarmers.
4. **Bicycle/**
   - `bicycle.c` - Implements the core function in the Bicycle Factory Problem.
   - `bicycle_main.c` - The program entrance.
   - `bicycle.h` - The header.
   - `Makefile` - A makefile script for Bicycle.

## How to Compile
Run the following command in the project directory:
```sh
make
```
This will generate all executable files in the `Prj2` directory.

## How to Clean
To remove all compiled executables, run:
```sh
make clean
```

## Notes
- Ensure you have `gcc` installed to compile the files.
- The executables will have the same names as their corresponding source files but without the `.c` extension.