# CSCE311Proj3

Implements a command-line utility that performs file operations entirely through
memory-mapped files using mmap, without traditional read/write system calls.
Supports three operations: create (opens/truncates a file and fills it with a
given character), insert (shifts existing bytes forward from an offset and writes
stdin bytes there), and append (writes stdin bytes to end-of-file incrementally,
mapping at most twice the current file size per iteration). All operations use
the provided proj3 wrapper functions. On error, insert and append restore the
file to its original size and contents.

# Files
- `main.cc` — implements create, insert, and append
- `README.md` — this file

# Build
mkdir -p bin obj
make

# Usage
bin/mmap_util create dat/data.bin A 1024
echo -n "Jasmine" | bin/mmap_util insert dat/data.bin 100 5
bin/mmap_util append dat/data.bin 1027 < dat/data_app.bin