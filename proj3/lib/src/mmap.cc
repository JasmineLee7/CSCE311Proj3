// Copyright Spring 2026 CSCE 311
//
#include "proj3/lib/include/mmap.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

[[noreturn]] void throw_errno(const char* func) {
  const int e = errno;
  throw std::system_error(
    e,
    std::generic_category(),
    std::string("proj3::") + func + " failed: " + std::strerror(e)
  );
}

} // namespace annonymous


int proj3::open(const char* pathname, int flags, ...) {
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list ap;
    va_start(ap, flags);
    mode = static_cast<mode_t>(va_arg(ap, int));
    va_end(ap);
  }

  int fd;
  if (flags & O_CREAT) {
    fd = ::open(pathname, flags, mode);
  } else {
    fd = ::open(pathname, flags);
  }

  if (fd == -1) {
    throw_errno("open");
  }
  return fd;
}

int proj3::close(int fd) {
  if (::close(fd) == -1) {
    throw_errno("close");
  }
  return 0;
}

int proj3::fstat(int fd, struct stat* buf) {
  if (::fstat(fd, buf) == -1) {
    throw_errno("fstat");
  }
  return 0;
}

int proj3::ftruncate(int fd, off_t length) {
  if (::ftruncate(fd, length) == -1) {
    throw_errno("ftruncate");
  }
  return 0;
}

int proj3::truncate(const char* path, off_t length) {
  if (::truncate(path, length) == -1) {
    throw_errno("truncate");
  }
  return 0;
}

void* proj3::mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  void* result = ::mmap(addr, length, prot, flags, fd, offset);
  if (result == MAP_FAILED) {
    throw_errno("mmap");
  }
  return result;
}

int proj3::munmap(void* addr, size_t length) {
  if (::munmap(addr, length) == -1) {
    throw_errno("munmap");
  }
  return 0;
}

int proj3::msync(void* addr, size_t length, int flags) {
  if (::msync(addr, length, flags) == -1) {
    throw_errno("msync");
  }
  return 0;
}
