// Copyright Spring 2026 CSCE 311
//
#ifndef PROJ3_LIB_MMAP_H_
#define PROJ3_LIB_MMAP_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <cstddef>


namespace proj3 {

// Use these to avoid including the necessary headers in your main.cc file.
// Where in proj3/bin/mmap_util process would these be stored?
constexpr int O_RDONLY = 0;
constexpr int O_WRONLY = 1;
constexpr int O_RDWR   = 2;

constexpr int O_CREAT  = 0100;
constexpr int O_EXCL   = 0200;
constexpr int O_TRUNC  = 01000;
constexpr int O_APPEND = 02000;

constexpr int PROT_NONE  = 0x0;
constexpr int PROT_READ  = 0x1;
constexpr int PROT_WRITE = 0x2;
constexpr int PROT_EXEC  = 0x4;

constexpr int MAP_SHARED  = 0x01;
constexpr int MAP_PRIVATE = 0x02;

constexpr int MS_ASYNC      = 1;
constexpr int MS_INVALIDATE = 2;
constexpr int MS_SYNC       = 4;


/*
 * open
 * ----
 * Opens a file and returns a file descriptor.
 *
 * pathname: path to the file
 * flags: access mode and options (e.g., O_RDONLY, O_RDWR, O_CREAT)
 * mode: (optional, required if O_CREAT is set) file permissions
 *
 * Returns:
 *   non-negative file descriptor on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - File descriptor is used in subsequent system calls.
 *   - Flags:
 *     + Creation: O_RDWR | O_CREAT | O_TRUNC
 *     + All others: O_RDWR
 */
int open(const char* pathname, int flags, ...);


/*
 * close
 * -----
 * Closes an open file descriptor.
 *
 * fd: file descriptor to close
 *
 * Returns:
 *   0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - Does not unmap memory created with mmap.
 *   - After closing, fd must not be used again.
 */
int close(int fd);


/*
 * fstat
 * -----
 * Retrieves metadata about an open file.
 *
 * fd: file descriptor
 * buf: pointer to struct stat to populate
 *
 * Returns:
 *   0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - Commonly used before resizing or mapping.
 *   - buf->st_size gives the current file size.
 *   - Example:
 *     ::stat f_stat;
 *     proj3::fstat(fd, &f_stat);
 *     ssize_t f_len = static_cast<ssize_t>(f_stat.st_size);
 *     std::cout << "File length: " << f_len << std::endl;*
 */
int fstat(int fd, struct stat* buf);


/*
 * ftruncate
 * ---------
 * Changes the size of an open file.
 *
 * fd: file descriptor
 * length: new file size in bytes
 *
 * Returns:
 *    0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - If length is larger, the file is extended (NEW BYTES ARE ZEROED).
 *   - If smaller, the file is truncated and data beyond length is lost.
 *   - Required before writing beyond current file size with mmap.
 */
int ftruncate(int fd, ::off_t length);


/*
 * mmap
 * ----
 * Maps a file or anonymous memory into the process address space.
 * Parameters:
 *   SEE BELOW
 *
 * Returns:
 *   pointer to mapped memory on success
 *   MAP_FAILED on failure (errno set)
 *
 * Notes:
 *   - Accessing the returned memory reads/writes the file.
 *   - MAP_SHARED propagates changes to the file (with msync or on unmap).
 *   - Mapping does not change file size.
 */
void* mmap(void* addr,  // desired mapping address (nullptr lets kernel choose)
           std::size_t length,  // numbe of bytes to map from addr
           int prot,  // memory protection (e.g., PROT_READ, PROT_WRITE)
           int flags,  // mapping type (e.g., MAP_SHARED, MAP_PRIVATE)
           int fd,  // file descriptor
           ::off_t offset);  // file offset (must be page-aligned)


/*
 * munmap
 * ------
 * Unmaps a previously mapped memory region.
 *
 * addr: pointer returned by mmap
 * length: size of the mapping
 *
 * Returns:
 *   0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - After unmapping, the memory must not be accessed.
 *   - Does not close the file descriptor; (call close on fd used in mmap)
 */
int munmap(void* addr, std::size_t length);


/*
 * msync
 * -----
 * Synchronizes a memory-mapped region with the underlying file.
 *
 * addr: start of mapped region
 * length: number of bytes to flush
 * flags: MS_SYNC (blocking) or MS_ASYNC (non-blocking)
 *
 * Returns:
 *    0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - Ensures that modifications are written to disk.
 *   - Required for deterministic persistence with MAP_SHARED.
 *   - MS_SYNC blocks until changes are committed.
 */
int msync(void* addr, std::size_t length, int flags);


/*
 * truncate
 * --------
 * Changes the size of a file by path.
 *
 * path: file path
 * length: new file size in bytes
 *
 * Returns:
 *    0 on success
 *   -1 on failure (errno set)
 *
 * Notes:
 *   - You probably do not want to use this, but I included for sake of
 *     completeness
 *   - Same behavior as ftruncate, but operates on a pathname.
 *   - Less commonly used when a file descriptor is already available.
 */
int truncate(const char* path, ::off_t length);

} // namespace proj3

#endif // PROJ3_LIB_MMAP_H_
