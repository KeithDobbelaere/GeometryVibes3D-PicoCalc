#include "PicoFileSystem.hpp"

extern "C" {
#include "sdcard.h"
#include "fat32.h"
}

namespace gv {

bool PicoFile::read(void* dst, size_t bytes, size_t& outRead) {
    outRead = 0;
    if (!f_) return false;
    outRead = std::fread(dst, 1, bytes, f_);
    return true;
}

bool PicoFile::seek(size_t absOffset) {
    if (!f_) return false;
    return std::fseek(f_, (long)absOffset, SEEK_SET) == 0;
}

size_t PicoFile::tell() const {
    if (!f_) return 0;
    long p = std::ftell(f_);
    return (p < 0) ? 0 : (size_t)p;
}

void PicoFile::close() {
    if (f_) {
        std::fclose(f_);
        f_ = nullptr;
    }

    // This file wrapper is heap-allocated by PicoFileSystem::openRead().
    delete this;
}

bool PicoFileSystem::init() {
    if (inited_) return true;

    sd_init();
    if (sd_card_init() != SD_OK) return false;

    fat32_init();
    if (fat32_mount() != FAT32_OK) return false;

    inited_ = true;
    return true;
}

IFile* PicoFileSystem::openRead(const char* path) {
    if (!inited_) return nullptr;

    FILE* f = std::fopen(path, "rb");
    if (!f) return nullptr;

    // Each open returns a distinct wrapper so multiple files can be open concurrently.
    return new PicoFile(f);
}

} // namespace gv