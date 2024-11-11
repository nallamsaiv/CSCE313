#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>
#include <vector>
#include <chrono>

void copyFile(const std::string &src, const std::string &dest, size_t buffer_size) {
    int src_fd, dest_fd;
    ssize_t bytes_read, bytes_written;
    std::vector<char> buffer(buffer_size);

    src_fd = open(src.c_str(), O_RDONLY);
    if (src_fd < 0) {
        perror("Error opening source file");
        exit(1);
    }

    dest_fd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (dest_fd < 0) {
        perror("Error opening/creating destination file");
        close(src_fd);
        exit(1);
    }

    while ((bytes_read = read(src_fd, buffer.data(), buffer_size)) > 0) {
        bytes_written = write(dest_fd, buffer.data(), bytes_read);
    }

    close(src_fd);
    close(dest_fd);
}


int main(int argc, char *argv[]) {
    std::string src = argv[1];
    std::string dest = argv[2];
    std::vector<size_t> buffer_sizes = {2048};

    for (size_t buffer_size : buffer_sizes) {
        copyFile(src, dest, buffer_size);
    }

    return 0;
}
