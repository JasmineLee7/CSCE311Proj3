// Copyright Jasmine Lee 2025
#include <proj3/lib/include/mmap.h>

#include <iostream>
#include <algorithm>

int Create(char *argv[]);
int Insert(char *argv[]);
int Append(char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage Cases:\n" 
            "--Create--  " << argv[0] << " create <path> <fill_char> <size>\n" 
            "--Insert--  " << argv[0] << " insert <path> <offset> <bytes_incoming> < stdin\n" 
            "--Append--  " << argv[0] << " append <path> <bytes_incoming> < stdin" << std::endl;
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
    const char *path = argv[2];
    char fill_char = argv[3][0];
    std::size_t size = std::atoi(argv[4]);

    int fd = proj3::open(path, proj3::O_RDWR | proj3::O_CREAT | proj3::O_TRUNC,
                         S_IRUSR | S_IWUSR);
    if (fd == -1) {
        return 1;
    }

    if (proj3::ftruncate(fd, size) == -1) {
        proj3::close(fd);
        return 1;
    }

    if (size == 0) {
        proj3::close(fd);
        return 0;
    }

    void *map = proj3::mmap(nullptr, size,
                            proj3::PROT_READ | proj3::PROT_WRITE,
                            proj3::MAP_SHARED, fd, 0);
    if (map == reinterpret_cast<void *>(-1)) {
        proj3::ftruncate(fd, 0);
        proj3::close(fd);
        return 1;
    }

    char *data = static_cast<char *>(map);
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = fill_char;
    }

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
    const char *path = argv[2];
    std::size_t offset = std::atoi(argv[3]);
    std::size_t bytes_incoming = std::atoi(argv[4]);

    int fd = proj3::open(path, proj3::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    struct stat f_stat;
    if (proj3::fstat(fd, &f_stat) == -1) {
        proj3::close(fd);
        return 1;
    }
    std::size_t f_len = static_cast<std::size_t>(f_stat.st_size);

    if (offset > f_len) {
        proj3::close(fd);
        return 1;
    }

    std::size_t new_size = f_len + bytes_incoming;

    if (proj3::ftruncate(fd, new_size) == -1) {
        proj3::close(fd);
        return 1;
    }

    void *map = proj3::mmap(nullptr, new_size,
                            proj3::PROT_READ | proj3::PROT_WRITE,
                            proj3::MAP_SHARED, fd, 0);
    if (map == reinterpret_cast<void *>(-1)) {
        proj3::ftruncate(fd, f_len);
        proj3::close(fd);
        return 1;
    }

    char *data = static_cast<char *>(map);

    for (std::size_t i = f_len; i > offset; --i) {
        data[(i - 1) + bytes_incoming] = data[i - 1];
    }

    for (std::size_t i = offset; i < offset + bytes_incoming; ++i) {
        int c = std::cin.get();
        if (c == EOF) {
            proj3::munmap(map, new_size);
            proj3::ftruncate(fd, f_len);
            proj3::close(fd);
            return 1;
        }
        data[i] = static_cast<char>(c);
    }

    if (proj3::msync(map, new_size, proj3::MS_SYNC) == -1) {
        proj3::munmap(map, new_size);
        proj3::ftruncate(fd, f_len);
        proj3::close(fd);
        return 1;
    }

    if (proj3::munmap(map, new_size) == -1) {
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
    const char *path = argv[2];
    std::size_t bytes_incoming = std::atoi(argv[3]);

    int fd = proj3::open(path, proj3::O_RDWR);
    if (fd == -1) {
        return 1;
    }

    struct stat f_stat;
    if (proj3::fstat(fd, &f_stat) == -1) {
        proj3::close(fd);
        return 1;
    }

    std::size_t orig_size = static_cast<std::size_t>(f_stat.st_size);
    std::size_t file_size = orig_size;
    std::size_t bytes_remaining = bytes_incoming;

    while (bytes_remaining > 0) {
        std::size_t bytes_to_append;
        if (file_size == 0) {
            bytes_to_append = 1;
        } else {
            bytes_to_append = std::min(file_size, bytes_remaining);
        }

        std::size_t new_size = file_size + bytes_to_append;

        if (proj3::ftruncate(fd, new_size) == -1) {
            proj3::ftruncate(fd, orig_size);
            proj3::close(fd);
            return 1;
        }

        void *map = proj3::mmap(nullptr, new_size,
                                proj3::PROT_READ | proj3::PROT_WRITE,
                                proj3::MAP_SHARED, fd, 0);
        if (map == reinterpret_cast<void *>(-1)) {
            proj3::ftruncate(fd, orig_size);
            proj3::close(fd);
            return 1;
        }

        char *data = static_cast<char *>(map);

        for (std::size_t i = file_size; i < new_size; ++i) {
            int c = std::cin.get();
            if (c == EOF) {
                proj3::munmap(map, new_size);
                proj3::ftruncate(fd, orig_size);
                proj3::close(fd);
                return 1;
            }
            data[i] = static_cast<char>(c);
        }

        if (proj3::msync(map, new_size, proj3::MS_SYNC) == -1) {
            proj3::munmap(map, new_size);
            proj3::ftruncate(fd, orig_size);
            proj3::close(fd);
            return 1;
        }

        if (proj3::munmap(map, new_size) == -1) {
            proj3::ftruncate(fd, orig_size);
            proj3::close(fd);
            return 1;
        }

        bytes_remaining -= bytes_to_append;
        file_size = new_size;
    }

    if (proj3::close(fd) == -1) {
        return 1;
    }

    return 0;
}