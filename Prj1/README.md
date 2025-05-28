# Project 1: Implementing Concurrency with Linux Processes and Threads

## Submitted Files

1. `Makefile` - A build script that compiles all C source files into executables.
2. `prj1_report.pdf` - A detailed report explaining the project.
3. **Copy/**
   - `MyCopy.c` - Implements a basic file copying function.
   - `ForkCopy.c` - Implements file copying using the `fork()` system call.
   - `PipeCopy.c` - Implements file copying using inter-process communication (pipes).
4. **Shell/**
   - `shell.c` - A command-line shell implementation that operates as a server.
5. **Sort/**
   - `MergesortSingle.c` - Implements a single-threaded merge sort algorithm.
   - `MergesortMulti.c` - Implements a multi-threaded merge sort algorithm. 

## How to Compile
Run the following command in the project directory:
```sh
make
```
This will generate all executable files in the `Prj1` directory.

## How to Clean
To remove all compiled executables, run:
```sh
make clean
```

## Notes
- Ensure you have `gcc` installed to compile the files.
- The executables will have the same names as their corresponding source files but without the `.c` extension.