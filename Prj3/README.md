# Project 3: Design and Implementation of File System 

## Submitted Files

1. ``Makefile`` - A build script that compiles all C source files into executables.

2. ``prj3_report.pdf`` - The project specification document that outlines objectives, requirements, implementation steps, and testing procedures.

3. **disk/**
   1. **include/**
       - `disk.h` - Header file that declares the disk module's public API, including initialization, command handlers, and cleanup.
    2. **src/**
        - `client.c` - A simple client that connects to the disk server, sends commands, and displays the responses.
        - `server.c` - Implements the disk server that simulates a physical disk with support for sector-based read/write and track-to-track seek delay.
        - `main.c` - Provides a command-line interface to interact directly with the disk module for testing purposes.
        - `disk.c` - Contains core logic for simulating a disk, including initialization, reading, writing, and memory-mapped file operations.
    3. **tests/**: Code files for function test.
    4. ``Makefile`` - A build script that compiles all C source files in directory **disk/**.

4. **fs/**
   1. **include/**
      - `inode.h` - Defines the structure and operations of both on-disk and in-memory inodes, which form the core of the file system metadata.
      - `block.h` - Handles block-level management and caching, including superblock definition, block allocation, and disk geometry settings.
      - `common.h` - Contains common type definitions, constants, and error codes used throughout the file system.
      - `fs.h` - Declares the main file system interface functions, supporting file/directory operations, access control, and path management.
    2. **src/**
        - `client.c` - Implements a disk client program that sends user commands to the disk server and displays the server's responses.
        - `server.c` - Implements a disk server that simulates a physical disk with seek delay and handles read/write requests.
        - `disk.c` - Provides low-level operations for a simulated disk, including sector read/write, initialization, and memory-mapped file management.
    3. **tests/** - Code files for function test.
    4. ``Makefile`` - A build script that compiles all C source files in directory **fs/**.

5. **include/** & **lib/** - Pre-defined header files and libraries.

## How to Compile
Run the following command in the project directory:
```sh
make
```
This will generate all executable files in the `Prj3` directory.

## How to test
Enter directory `disk` or `fs` and then run the following command:
```sh
make test
```
This will check if the files in the disk or file system module can be compiled and run normally. 

Passing the tests ***does not*** guarantee that the function is correct; it only confirms that the program did not encounter any fatal errors with the given test cases.

## How to Clean
To remove all compiled executables, run:
```sh
make clean
```

## Notes
- Ensure you have `gcc` installed to compile the files.
- The executables will have the same names as their corresponding source files but without the `.c` extension.
