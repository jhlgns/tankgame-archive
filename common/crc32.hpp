#pragma once

#include "common.hpp"

struct Crc32 {
    static u32 table[256];
    u32 value = 0;

    static void InitTable() {
        constexpr u32 polynomial = 0xEDB88320;

        for (u32 i = 0; i < 256; i++) {
            u32 c = i;

            for (size_t j = 0; j < 8; j++) {
                if (c & 1) {
                    c = polynomial ^ (c >> 1);
                } else {
                    c >>= 1;
                }
            }

            Crc32::table[i] = c;
        }
    }

    template<typename T>
    void Add(const T& data) {
        u32 c = this->value ^ 0xFFFFFFFF;

        for (size_t i = 0; i < sizeof(T); ++i) {
            c = Crc32::table[(c ^ reinterpret_cast<const u8 *>(&data)[i]) & 0xFF] ^ (c >> 8);
        }

        this->value = c ^ 0xFFFFFFFF;
    }

    template<typename T1, typename T2, typename... Ts>
    void Add(const T1 &data1, const T2 &data2, const Ts &...rest) {
        this->Add(data1);
        this->Add(data2);
        (this->Add(rest), ...);
    }
};

