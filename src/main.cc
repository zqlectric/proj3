#include "../include/main.h"


// Get current file size using proj3::fstat
static off_t file_size(int fd) {
  struct stat st;
  proj3::fstat(fd, &st);
  return st.st_size;
}

// create <path> <fill_char> <size>
static int do_create(const char* path, char fill_char, off_t size) {
  int fd = proj3::open(path,
                       proj3::O_RDWR | proj3::O_CREAT | proj3::O_TRUNC,
                       0644);
  proj3::ftruncate(fd, size);

  if (size > 0) {
    void* addr = proj3::mmap(nullptr,
                             static_cast<std::size_t>(size),
                             proj3::PROT_READ | proj3::PROT_WRITE,
                             proj3::MAP_SHARED,
                             fd, 0);
    std::memset(addr, static_cast<unsigned char>(fill_char),
                static_cast<std::size_t>(size));
    proj3::msync(addr, static_cast<std::size_t>(size), proj3::MS_SYNC);
    proj3::munmap(addr, static_cast<std::size_t>(size));
  }

  proj3::close(fd);
  return 0;
}

// insert <path> <offset> <bytes_incoming>
static int do_insert(const char* path, off_t offset, off_t bytes_incoming) {
  int fd = proj3::open(path, proj3::O_RDWR);
  off_t orig_size = file_size(fd);

  if (offset > orig_size) {
    std::cerr << "mmap_util insert: offset " << offset
              << " exceeds file size " << orig_size << "\n";
    proj3::close(fd);
    return 1;
  }

  std::vector<uint8_t> incoming(static_cast<std::size_t>(bytes_incoming));
  off_t total_read = 0;
  while (total_read < bytes_incoming) {
    std::streamsize want =
        static_cast<std::streamsize>(bytes_incoming - total_read);
    std::streamsize got = 0;
    if (!std::cin.read(reinterpret_cast<char*>(incoming.data() + total_read),
                       want)) {
      got = std::cin.gcount();
    } else {
      got = want;
    }
    total_read += got;
    if (total_read < bytes_incoming && std::cin.eof()) {
      std::cerr << "mmap_util insert: stdin ended after " << total_read
                << " bytes (expected " << bytes_incoming << ")\n";
      proj3::close(fd);
      return 1;
    }
  }

  std::vector<uint8_t> snapshot;
  if (orig_size > 0) {
    snapshot.resize(static_cast<std::size_t>(orig_size));
    void* snap_addr = proj3::mmap(nullptr,
                                  static_cast<std::size_t>(orig_size),
                                  proj3::PROT_READ,
                                  proj3::MAP_SHARED,
                                  fd, 0);
    std::memcpy(snapshot.data(), snap_addr,
                static_cast<std::size_t>(orig_size));
    proj3::munmap(snap_addr, static_cast<std::size_t>(orig_size));
  }

  auto restore = [&]() -> int {
    try {
      proj3::ftruncate(fd, orig_size);
      if (orig_size > 0) {
        void* addr = proj3::mmap(nullptr,
                                 static_cast<std::size_t>(orig_size),
                                 proj3::PROT_READ | proj3::PROT_WRITE,
                                 proj3::MAP_SHARED,
                                 fd, 0);
        std::memcpy(addr, snapshot.data(),
                    static_cast<std::size_t>(orig_size));
        proj3::msync(addr, static_cast<std::size_t>(orig_size),
                     proj3::MS_SYNC);
        proj3::munmap(addr, static_cast<std::size_t>(orig_size));
      }
    } catch (...) {}
    proj3::close(fd);
    return 1;
  };

  try {
    off_t new_size = orig_size + bytes_incoming;
    proj3::ftruncate(fd, new_size);

    void* addr = proj3::mmap(nullptr,
                             static_cast<std::size_t>(new_size),
                             proj3::PROT_READ | proj3::PROT_WRITE,
                             proj3::MAP_SHARED,
                             fd, 0);

    uint8_t* buf = static_cast<uint8_t*>(addr);

    if (offset < orig_size) {
      std::memmove(buf + offset + bytes_incoming,
                   buf + offset,
                   static_cast<std::size_t>(orig_size - offset));
    }

    std::memcpy(buf + offset, incoming.data(),
                static_cast<std::size_t>(bytes_incoming));

    proj3::msync(addr, static_cast<std::size_t>(new_size), proj3::MS_SYNC);
    proj3::munmap(addr, static_cast<std::size_t>(new_size));
  } catch (...) {
    return restore();
  }

  proj3::close(fd);
  return 0;
}


// append <path> <bytes_incoming>
static int do_append(const char* path, off_t bytes_incoming) {
  int fd = proj3::open(path, proj3::O_RDWR);
  off_t orig_size = file_size(fd);

  std::vector<uint8_t> snapshot;
  if (orig_size > 0) {
    snapshot.resize(static_cast<std::size_t>(orig_size));
    void* snap_addr = proj3::mmap(nullptr,
                                  static_cast<std::size_t>(orig_size),
                                  proj3::PROT_READ,
                                  proj3::MAP_SHARED,
                                  fd, 0);
    std::memcpy(snapshot.data(), snap_addr,
                static_cast<std::size_t>(orig_size));
    proj3::munmap(snap_addr, static_cast<std::size_t>(orig_size));
  }

  auto restore = [&]() -> int {
    try {
      proj3::ftruncate(fd, orig_size);
      if (orig_size > 0) {
        void* addr = proj3::mmap(nullptr,
                                 static_cast<std::size_t>(orig_size),
                                 proj3::PROT_READ | proj3::PROT_WRITE,
                                 proj3::MAP_SHARED,
                                 fd, 0);
        std::memcpy(addr, snapshot.data(),
                    static_cast<std::size_t>(orig_size));
        proj3::msync(addr, static_cast<std::size_t>(orig_size),
                     proj3::MS_SYNC);
        proj3::munmap(addr, static_cast<std::size_t>(orig_size));
      }
    } catch (...) {}
    proj3::close(fd);
    return 1;
  };

  off_t total_appended = 0;
  off_t remaining      = bytes_incoming;

  while (remaining > 0) {
    off_t current_size = orig_size + total_appended;

    off_t chunk_size;
    if (current_size == 0) {
      chunk_size = 1;
    } else {
      chunk_size = current_size;
      if (chunk_size > remaining)
        chunk_size = remaining;
    }

    std::vector<uint8_t> buf(static_cast<std::size_t>(chunk_size));
    off_t got = 0;
    while (got < chunk_size) {
      std::streamsize want = static_cast<std::streamsize>(chunk_size - got);
      std::streamsize n = 0;
      if (!std::cin.read(reinterpret_cast<char*>(buf.data() + got), want)) {
        n = std::cin.gcount();
      } else {
        n = want;
      }
      got += n;
      if (got < chunk_size && std::cin.eof()) {
        std::cerr << "mmap_util append: stdin ended prematurely after "
                  << (total_appended + got) << " bytes (expected "
                  << bytes_incoming << ")\n";
        return restore();
      }
    }

    try {
      off_t new_size = current_size + chunk_size;
      proj3::ftruncate(fd, new_size);

      const off_t page_size = static_cast<off_t>(::getpagesize());
      off_t map_offset      = (current_size / page_size) * page_size;
      off_t delta           = current_size - map_offset;
      off_t map_length      = delta + chunk_size;

      void* addr = proj3::mmap(nullptr,
                               static_cast<std::size_t>(map_length),
                               proj3::PROT_READ | proj3::PROT_WRITE,
                               proj3::MAP_SHARED,
                               fd,
                               map_offset);

      uint8_t* dest = static_cast<uint8_t*>(addr) + delta;
      std::memcpy(dest, buf.data(), static_cast<std::size_t>(chunk_size));

      proj3::msync(addr, static_cast<std::size_t>(map_length), proj3::MS_SYNC);
      proj3::munmap(addr, static_cast<std::size_t>(map_length));
    } catch (...) {
      return restore();
    }

    total_appended += chunk_size;
    remaining      -= chunk_size;
  }

  proj3::close(fd);
  return 0;
}

// main
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage:\n"
              << "  mmap_util create <path> <fill_char> <size>\n"
              << "  mmap_util insert <path> <offset> <bytes_incoming>\n"
              << "  mmap_util append <path> <bytes_incoming>\n";
    return 1;
  }

  std::string cmd(argv[1]);

  try {
    if (cmd == "create") {
      if (argc != 5) {
        std::cerr << "Usage: mmap_util create <path> <fill_char> <size>\n";
        return 1;
      }
      if (std::strlen(argv[3]) != 1) {
        std::cerr << "mmap_util create: fill_char must be exactly one byte\n";
        return 1;
      }
      char  fill_char = argv[3][0];
      off_t size      = static_cast<off_t>(std::stoll(argv[4]));
      if (size < 0) {
        std::cerr << "mmap_util create: size must be non-negative\n";
        return 1;
      }
      return do_create(argv[2], fill_char, size);

    } else if (cmd == "insert") {
      if (argc != 5) {
        std::cerr << "Usage: mmap_util insert <path> <offset> <bytes_incoming>\n";
        return 1;
      }
      off_t offset         = static_cast<off_t>(std::stoll(argv[3]));
      off_t bytes_incoming = static_cast<off_t>(std::stoll(argv[4]));
      if (offset < 0 || bytes_incoming < 0) {
        std::cerr << "mmap_util insert: arguments must be non-negative\n";
        return 1;
      }
      return do_insert(argv[2], offset, bytes_incoming);

    } else if (cmd == "append") {
      if (argc != 4) {
        std::cerr << "Usage: mmap_util append <path> <bytes_incoming>\n";
        return 1;
      }
      off_t bytes_incoming = static_cast<off_t>(std::stoll(argv[3]));
      if (bytes_incoming < 0) {
        std::cerr << "mmap_util append: bytes_incoming must be non-negative\n";
        return 1;
      }
      return do_append(argv[2], bytes_incoming);

    } else {
      std::cerr << "mmap_util: unknown command '" << cmd << "'\n";
      return 1;
    }

  } catch (const std::exception& e) {
    std::cerr << "mmap_util: " << e.what() << "\n";
    return 1;
  }

  return 0;
}