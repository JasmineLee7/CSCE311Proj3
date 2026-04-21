#include <proj3/lib/include/mmap.h>

#include <iostream>
#include <algorithm>
#include <sys/stat.h>

int Create(char *argv[]);
int Insert(char *argv[]);
int Append(char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage Cases:\n" <<
            "--Create--  " << argv[0] << " create <file path> " <<
            "<fill_char> <size>\n" <<
            "--Insert--  " << argv[0] << " insert <file path> " <<
            "<offset> <bytes_incoming> < stdin\n" <<
            "--Append--  " << argv[0] << " append <file path> " <<
            "<bytes_incoming> < stdin" << std::endl;
        return 0;
    }

    std::string action = argv[1];

    if (action == "create") {
        return Create(argv);
    } else if (action == "insert") {
        return Insert(argv);
    } else if (action == "append") {
        return Append(argv);
    }

    return 0;
}

int Create(char *argv[]) {
    const char *filePath = argv[2];
    char fillChar = argv[3][0];
    std::size_t size = std::atoi(argv[4]);

    // Open or create file, truncating any existing content
    int fd = proj3::open(filePath, proj3::O_RDWR | proj3::O_CREAT | proj3::O_TRUNC,
                         S_IRUSR | S_IWUSR);
    if (fd == -1) {
        return 1;
    }

    // Set file to requested size
    if (proj3::ftruncate(fd, size) == -1) {
        proj3::close(fd);
        return 1;
    }

    // A size of 0 means nothing to map or fill
    if (size == 0) {
        proj3::close(fd);
        return 0;
    }

    // Map the file into memory
    void *map = proj3::mmap(nullptr, size,
                            proj3::PROT_READ | proj3::PROT_WRITE,
                            proj3::MAP_SHARED, fd, 0);
    if (map == reinterpret_cast<void *>(-1)) {
        proj3::ftruncate(fd, 0);
        proj3::close(fd);
        return 1;
    }

    // Fill every byte with fill_char
    char *charMap = static_cast<char *>(map);
    for (std::size_t i = 0; i < size; ++i) {
        charMap[i] = fillChar;
    }

    // Sync, unmap, close
    if (proj3::msync(map, size, proj3::MS_SYNC) == -1) {
        proj3::munmap(map, size);
        proj3::close(fd);
        return 1;
    }

    if (proj3::munmap(map, size) == -1) {
        proj3::close(fd);
        return 1;
    }

    if (proj3::close(fd) == -1) {
        return 1;
    }

    return 0;
}

int Insert(char *argv[]) {
    const char *filePath = argv[2];
    std::size_t offset = std::atoi(argv[3]);
    std::size_t bytesIncoming = std::atoi(argv[4]);

    int fd = proj3::open(filePath, proj3::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    // Get current file size
    struct stat f_stat;
    if (proj3::fstat(fd, &f_stat) == -1) {
        proj3::close(fd);
        return 1;
    }
    std::size_t f_len = static_cast<std::size_t>(f_stat.st_size);

    // Offset must not exceed file size
    if (offset > f_len) {
        proj3::close(fd);
        return 1;
    }

    std::size_t newSize = f_len + bytesIncoming;

    // Expand file to fit incoming bytes
    if (proj3::ftruncate(fd, newSize) == -1) {
        proj3::close(fd);
        return 1;
    }

    // Map the expanded file
    void *map = proj3::mmap(nullptr, newSize,
                            proj3::PROT_READ | proj3::PROT_WRITE,
                            proj3::MAP_SHARED, fd, 0);
    if (map == reinterpret_cast<void *>(-1)) {
        proj3::ftruncate(fd, f_len);
        proj3::close(fd);
        return 1;
    }

    char *charMap = static_cast<char *>(map);

    // Shift existing bytes after offset forward to make room
    for (std::size_t i = f_len; i > offset; --i) {
        charMap[(i - 1) + bytesIncoming] = charMap[i - 1];
    }

    // Write incoming bytes from stdin at offset
    for (std::size_t i = offset; i < offset + bytesIncoming; ++i) {
        int byte = std::cin.get();
        if (byte == EOF) {
            proj3::munmap(map, newSize);
            proj3::ftruncate(fd, f_len);
            proj3::close(fd);
            return 1;
        }
        charMap[i] = static_cast<char>(byte);
    }

    // Sync, unmap, close
    if (proj3::msync(map, newSize, proj3::MS_SYNC) == -1) {
        proj3::munmap(map, newSize);
        proj3::ftruncate(fd, f_len);
        proj3::close(fd);
        return 1;
    }

    if (proj3::munmap(map, newSize) == -1) {
        proj3::ftruncate(fd, f_len);
        proj3::close(fd);
        return 1;
    }

    if (proj3::close(fd) == -1) {
        return 1;
    }

    return 0;
}

int Append(char *argv[]) {
    const char *filePath = argv[2];
    std::size_t bytesIncoming = std::atoi(argv[3]);

    int fd = proj3::open(filePath, proj3::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    // Get current file size
    struct stat f_stat;
    if (proj3::fstat(fd, &f_stat) == -1) {
        proj3::close(fd);
        return 1;
    }
    std::size_t originalSize = static_cast<std::size_t>(f_stat.st_size);
    std::size_t currentSize = originalSize;
    std::size_t remaining = bytesIncoming;

    // Process bytes incrementally; each mapping must not exceed 2x file size at mapping time
    while (remaining > 0) {
        // How many bytes to append this iteration
        std::size_t chunkSize;
        if (currentSize == 0) {
            // File is empty, map at least 1 byte
            chunkSize = 1;
        } else {
            chunkSize = std::min(currentSize, remaining);
        }

        std::size_t newSize = currentSize + chunkSize;

        if (proj3::ftruncate(fd, newSize) == -1) {
            proj3::ftruncate(fd, originalSize);
            proj3::close(fd);
            return 1;
        }

        // Map only the new region (offset = currentSize)
        // newSize <= 2 * currentSize when currentSize > 0 (chunkSize <= currentSize)
        // When currentSize == 0, newSize == 1 which is acceptable
        void *map = proj3::mmap(nullptr, newSize,
                                proj3::PROT_READ | proj3::PROT_WRITE,
                                proj3::MAP_SHARED, fd, 0);
        if (map == reinterpret_cast<void *>(-1)) {
            proj3::ftruncate(fd, originalSize);
            proj3::close(fd);
            return 1;
        }

        char *charMap = static_cast<char *>(map);

        // Read chunkSize bytes from stdin into the new portion of the file
        for (std::size_t i = currentSize; i < newSize; ++i) {
            int byte = std::cin.get();
            if (byte == EOF) {
                proj3::munmap(map, newSize);
                proj3::ftruncate(fd, originalSize);
                proj3::close(fd);
                return 1;
            }
            charMap[i] = static_cast<char>(byte);
        }

        if (proj3::msync(map, newSize, proj3::MS_SYNC) == -1) {
            proj3::munmap(map, newSize);
            proj3::ftruncate(fd, originalSize);
            proj3::close(fd);
            return 1;
        }

        if (proj3::munmap(map, newSize) == -1) {
            proj3::ftruncate(fd, originalSize);
            proj3::close(fd);
            return 1;
        }

        remaining -= chunkSize;
        currentSize = newSize;
    }

    if (proj3::close(fd) == -1) {
        return 1;
    }

    return 0;
}