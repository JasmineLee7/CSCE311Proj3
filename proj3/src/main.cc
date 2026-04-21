
#include <proj03/lib/include/mmap.h>

#include <iostream>
#include <algorithm>

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

    // Get the action the user wants to be done
    std::string action = argv[1];

    // Process for when a file wants to be created
    if (action == "create") {
        return Create(argv);
    }
    else if (action == "insert") {
        return Insert(argv);
    }
    else if (action == "append") {
        return Append(argv);
    }

    return 0;
}

// Need to error check mmap, msync, munmap, and close
int Create(char *argv[]) {
    // Get the file path, character to fill with, and
    // the size for the ftruncate and map functions
    const char *filePath = argv[2];
    char fillChar = argv[3][0];
    std::size_t sizeForTrunc = std::atoi(argv[4]);
    std::size_t sizeForMap = std::atoi(argv[4]);

    // Opens the file and resizes the file
    // BEST GUESS: flags/mode were partially obscured in the screenshots.
    int fd = proj03::open(filePath, proj03::O_RDWR | proj03::O_CREAT,
                          proj03::S_IRUSR | proj03::S_IWUSR);
    if (fd == -1) {
        return 1;
    }

    // BEST GUESS: variable name looked like fTrunc in the screenshot.
    int fTrunc = proj03::ftruncate(fd, sizeForTrunc);
    if (fTrunc == -1) {
        proj03::close(fd);
        return 1;
    }

    // Check if the size used for the map is not 0
    if (sizeForMap == 0) {
        proj03::close(fd);
        return 1;
    }

    // Maps either a file or an anonymous memory space
    // to the processed address space
    void *map = proj03::mmap(nullptr, sizeForMap,
                             proj03::PROT_READ | proj03::PROT_WRITE,
                             proj03::MAP_SHARED, fd, 0);

    // Check if map returned MAP_FAILED
    if (map == reinterpret_cast<void *>(-1)) {
        proj03::ftruncate(fd, 0);
        proj03::close(fd);
        return 1;
    }

    // Convert the output of mmap into a
    // character map and fill it with the fill char
    char *charMap = static_cast<char *>(map);
    for (std::size_t i = 0; i < sizeForMap; ++i) {
        charMap[i] = fillChar;
    }

    // Flush the changes and unmap the memory space
    // Finally, close the file
    if (proj03::msync(map, sizeForMap, proj03::MS_SYNC) == -1) {
        proj03::munmap(map, sizeForMap);
        proj03::close(fd);
        return 1;
    }

    if (proj03::munmap(map, sizeForMap) == -1) {
        proj03::close(fd);
        return 1;
    }

    if (proj03::close(fd) == -1) {
        return 1;
    }

    return 0;
}

// Check errors for mmap, msync, munmap, and close
int Insert(char *argv[]) {
    // Get the file path, offset and the number of bytes
    const char *filePath = argv[2];
    std::size_t offset = std::atoi(argv[3]);
    std::size_t bytesIncoming = std::atoi(argv[4]);

    // Open the file
    int fd = proj03::open(filePath, proj03::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    // Get the file size
    struct stat f_stat;
    if (proj03::fstat(fd, &f_stat) == -1) {
        proj03::close(fd);
        return 1;
    }
    std::size_t f_len = static_cast<std::size_t>(f_stat.st_size);

    // Check if the offset is greater than the length of the file
    if (offset > f_len) {
        proj03::close(fd);
        return 1;
    }

    // Get the new size of the file and resize the file
    std::size_t newSizeForFile = bytesIncoming + f_len;
    if (proj03::ftruncate(fd, newSizeForFile) == -1) {
        proj03::close(fd);
        return 1;
    }

    // Maps the file to a processed address space
    void *map = proj03::mmap(nullptr, newSizeForFile,
                             proj03::PROT_READ | proj03::PROT_WRITE,
                             proj03::MAP_SHARED, fd, 0);

    // Check if map returned MAP_FAILED
    if (map == reinterpret_cast<void *>(-1)) {
        proj03::ftruncate(fd, f_len);
        proj03::close(fd);
        return 1;
    }

    // Converts map to a char map
    char *charMap = static_cast<char *>(map);

    // Do a backwards loop to "make space" for
    // the new bytes to append to the file
    for (std::size_t i = f_len; i > offset; --i) {
        std::size_t oldPosition = i - 1;
        std::size_t newPosition = (oldPosition) + bytesIncoming;
        charMap[newPosition] = charMap[oldPosition];
    }

    // Inserts the new bytes to the file
    for (std::size_t i = offset; i < (offset + bytesIncoming); ++i) {
        int readByte = std::cin.get();
        if (readByte == EOF) {
            // Do not believe this needs error checking
            // since this is happening because of a previous error
            proj03::munmap(map, newSizeForFile);
            proj03::ftruncate(fd, f_len);
            proj03::close(fd);
            return 1;
        }
        charMap[i] = static_cast<char>(readByte);
    }

    // Flush the changes and unmap the memory space
    // and close the file
    if (proj03::msync(map, newSizeForFile, proj03::MS_SYNC) == -1) {
        proj03::munmap(map, newSizeForFile);
        proj03::ftruncate(fd, f_len);
        proj03::close(fd);
        return 1;
    }

    if (proj03::munmap(map, newSizeForFile) == -1) {
        proj03::ftruncate(fd, f_len);
        proj03::close(fd);
        return 1;
    }

    if (proj03::close(fd) == -1) {
        return 1;
    }

    return 0;
}

// Check errors for mmap, msync, munmap, and close
int Append(char *argv[]) {
    // Get file path and the number of incoming bytes
    const char *filePath = argv[2];
    std::size_t bytesIncoming = std::atoi(argv[3]);

    // Try to open a file
    int fd = proj03::open(filePath, proj03::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    // Try to get the file length, if fstat fails,
    // close file descriptor and return
    struct stat f_stat;
    if (proj03::fstat(fd, &f_stat) == -1) {
        proj03::close(fd);
        return 1;
    }
    std::size_t f_len = static_cast<std::size_t>(f_stat.st_size);

    // Keep track of the original file size, current file size,
    // and the remaining bytes to add to the file
    std::size_t originalFileSize = f_len;
    std::size_t currentFileSize = f_len;
    std::size_t remainingBytes = bytesIncoming;

    // If the size of the file is 0 and there is bytes remaining
    // Append the first byte and then go through while loop
    if (currentFileSize == 0 && remainingBytes > 0) {
        // Check if ftruncate fails for the appending the first byte
        if (proj03::ftruncate(fd, 1) == -1) {
            proj03::close(fd);
            return 1;
        }

        // Create map
        void *oneByteMap = proj03::mmap(nullptr, 1,
                                        proj03::PROT_READ | proj03::PROT_WRITE,
                                        proj03::MAP_SHARED, fd, 0);

        // Check if map returned MAP_FAILED
        if (oneByteMap == reinterpret_cast<void *>(-1)) {
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        char *charOneByteMap = static_cast<char *>(oneByteMap);

        // Get the first byte and check if its EOF
        int readOneByte = std::cin.get();

        // If so, unmap and revert the file size,
        // and close file descriptor
        if (readOneByte == EOF) {
            proj03::munmap(oneByteMap, 1);
            if (proj03::ftruncate(fd, originalFileSize) == -1) {
                proj03::close(fd);
                return 1;
            }
            proj03::close(fd);
            return 1;
        }

        // Append the single byte and update variables
        charOneByteMap[0] = static_cast<char>(readOneByte);
        if (proj03::msync(oneByteMap, 1, proj03::MS_SYNC) == -1) {
            proj03::munmap(oneByteMap, 1);
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        if (proj03::munmap(oneByteMap, 1) == -1) {
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        currentFileSize = 1;
        remainingBytes -= 1;
    }

    // Main loop to append bytes
    while (remainingBytes > 0) {
        // Get the minimum bytes to append to file so
        // it does not reach EOF
        std::size_t appendedBytes = std::min(currentFileSize, remainingBytes);
        std::size_t newFileSize = currentFileSize + appendedBytes;

        // Check if ftruncate fails
        if (proj03::ftruncate(fd, newFileSize) == -1) {
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        // Create map
        void *map = proj03::mmap(nullptr, newFileSize,
                                 proj03::PROT_READ | proj03::PROT_WRITE,
                                 proj03::MAP_SHARED, fd, 0);

        // Check if map returned MAP_FAILED
        if (map == reinterpret_cast<void *>(-1)) {
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        char *charMap = static_cast<char *>(map);

        // Append the remaining bytes
        for (std::size_t i = currentFileSize; i < newFileSize; ++i) {
            int readByte = std::cin.get();
            if (readByte == EOF) {
                // No need to error check I believe
                // since this is happening because of an error
                proj03::munmap(map, newFileSize);
                if (proj03::ftruncate(fd, originalFileSize) == -1) {
                    proj03::close(fd);
                    return 1;
                }
                proj03::close(fd);
                return 1;
            }
            charMap[i] = static_cast<char>(readByte);
        }

        // Flush changes and unmap memory
        // Update variables
        if (proj03::msync(map, newFileSize, proj03::MS_SYNC) == -1) {
            proj03::munmap(map, newFileSize);
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        if (proj03::munmap(map, newFileSize) == -1) {
            proj03::ftruncate(fd, originalFileSize);
            proj03::close(fd);
            return 1;
        }

        remainingBytes -= appendedBytes;
        currentFileSize = newFileSize;
    }

    // close the file
    if (proj03::close(fd) == -1) {
        return 1;
    }

    return 0;
}
