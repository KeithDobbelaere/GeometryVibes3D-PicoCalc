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

    // close() releases all resources for this file.
    // Implementations may self-delete; the pointer is invalid after close().
    virtual void close() = 0;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    // init() mounts or prepares the underlying storage.
    virtual bool init() = 0;

    // openRead() returns a distinct file object.
    // Multiple files may be open concurrently.
    // The returned pointer remains valid until close() is called.
    virtual IFile* openRead(const char* path) = 0; // returns nullptr on fail
};

} // namespace gv