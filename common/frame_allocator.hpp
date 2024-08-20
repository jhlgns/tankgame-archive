#pragma once

#include "common/common.hpp"

struct FrameAllocator {
    inline void Reserve(size_t num_bytes) {
        this->memory = std::make_unique<char[]>(num_bytes);
    }

    inline void *Alloc(size_t num_bytes) {
        if (this->water_mark + num_bytes < this->size) {
            assert(false);
            return nullptr;
        }

        auto res = this->memory.get() + this->water_mark;
        this->water_mark += num_bytes;
        return res;
    }

    UniquePtr<char[]> memory;
    size_t size = 0;
    size_t water_mark = 0;
};
