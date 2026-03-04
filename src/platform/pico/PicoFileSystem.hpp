#pragma once
#include "platform/IFileSystem.hpp"
#include <cstdio>

namespace gv {

class PicoFile final : public IFile {
public:
    explicit PicoFile(FILE* f) : f_(f) {}

    bool read(void* dst, size_t bytes, size_t& outRead) override;
    bool seek(size_t absOffset) override;
    size_t tell() const override;

    // close() closes the FILE and deletes this wrapper.
    void close() override;

private:
    FILE* f_ = nullptr;
};

class PicoFileSystem final : public IFileSystem {
public:
    bool init() override;
    IFile* openRead(const char* path) override;

private:
    bool inited_ = false;
};

} // namespace gv