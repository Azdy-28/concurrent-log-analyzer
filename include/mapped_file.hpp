#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>
#include <string>

class MappedFile {
public:
    explicit MappedFile(const std::string& path) {
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ == -1) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        struct stat st{};
        if (::fstat(fd_, &st) == -1) {
            ::close(fd_);
            throw std::runtime_error("fstat failed for: " + path);
        }
        size_ = static_cast<size_t>(st.st_size);
        if (size_ == 0) {
            data_ = nullptr;
            return;
        }
        void* mapped = ::mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0);
        if (mapped == MAP_FAILED) {
            ::close(fd_);
            throw std::runtime_error("mmap failed for: " + path);
        }
        data_ = static_cast<const char*>(mapped);
        ::madvise(mapped, size_, MADV_SEQUENTIAL);
    }

    ~MappedFile() {
        if (data_) ::munmap(const_cast<char*>(data_), size_);
        if (fd_ != -1) ::close(fd_);
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    [[nodiscard]] const char* data() const noexcept { return data_; }
    [[nodiscard]] size_t size() const noexcept { return size_; }

private:
    int fd_ = -1;
    const char* data_ = nullptr;
    size_t size_ = 0;
};
