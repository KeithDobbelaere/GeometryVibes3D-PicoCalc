#pragma once
#include <cstddef>
#include <cstdint>

namespace gv {

class IFile {
public:
    virtual ~IFile() = default;
    virtual bool read(void* dst, size_t bytes, size_t& outRead) = 0;
    virtual bool seek(size_t absOffset) = 0;
    virtual size_t tell() const = 0;
    virtual void close() = 0;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual bool init() = 0; // mount SD/FAT, etc.
    virtual IFile* openRead(const char* path) = 0; // returns nullptr on fail
};

} // namespace gv