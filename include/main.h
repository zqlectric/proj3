// Copyright Spring 2026 CSCE 311
#ifndef PROJ3_INCLUDE_MAIN_H_
#define PROJ3_INCLUDE_MAIN_H_

#include "../lib/include/mmap.h"

#include <sys/stat.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>


static off_t file_size(int fd);
static int do_create(const char* path, char fill_char, off_t size);
static int do_insert(const char* path, off_t offset, off_t bytes_incoming);
static int do_append(const char* path, off_t bytes_incoming);


#endif  // PROJ3_INCLUDE_MAIN_H_