# Project 3: Memory-Mapped File Manipulation

## Overview

This program is a command-line tool called `mmap_util`.

It works with files using memory mapping.
All file operations use the provided `proj3::` functions.

It has three commands:

* create
* insert
* append

---

## Build

Compile with:

```bash
make
```

---

## Usage

### Create

Creates a file or overwrites it. Fills it with one byte.

```bash
mmap_util create <path> <fill_char> <size>
```

Example:

```bash
mmap_util create data.bin A 1024
```

---

### Insert

Inserts bytes from stdin at a given offset.

```bash
mmap_util insert <path> <offset> <bytes_incoming>
```

Example:

```bash
echo -n "hello" | mmap_util insert data.bin 100 5
```

---

### Append

Adds bytes from stdin to the end of the file.

```bash
mmap_util append <path> <bytes_incoming>
```

Example:

```bash
mmap_util append data.bin 5 < input.bin
```

---

## Details

* Uses only these functions:

  * `proj3::open`, `proj3::close`
  * `proj3::fstat`, `proj3::ftruncate`, `proj3::truncate`
  * `proj3::mmap`, `proj3::munmap`, `proj3::msync`

* Does not use `read()` or `write()` on the file.

* stdin is treated as raw bytes.

* If `insert` or `append` fails, the file is restored.

* `append` processes input in chunks to follow the mmap size rule.

---

## Files

* `main.cc`
  Main program.

* `main.h`
  Function declarations.

* `README.md`
  This file.