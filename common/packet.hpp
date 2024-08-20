#pragma once

#include "common.hpp"

struct Packet_Header {
    u32 size;
};

#if 0
template<typename T>
inline T zigzag_e(T value) {
    return (value << 1) ^ (value >> (sizeof(T) * 8 - 1));
}

template<typename T>
inline T zigzag_d(T value) {
    return -(value & 1) ^ (value >> 1);
}
#endif


struct Packet {
    inline Packet()
        : position(sizeof(Packet_Header))
        , valid(true) {
        this->buffer.resize(sizeof(Packet_Header));
    }

    inline void Reset(Array<char> &&buffer) {
        *this = Packet{};
        this->buffer = ToRvalue(buffer);
    }

    inline void WriteHeader() {
        assert(this->position >= sizeof(Packet_Header));
        assert(this->position == this->buffer.size());

        Packet_Header hdr;
        hdr.size = this->position;
        std::memcpy(&this->buffer[0], &hdr, sizeof(hdr));
    }

    inline void WriteData(const void *data, u32 size) {
        if (size == 0) {
            return;
        }

        //assert(this->pos + size <= this->buf.size());
        assert(this->position == this->buffer.size());
        this->buffer.resize(static_cast<size_t>(this->position) + size);

        std::memcpy(&this->buffer[this->position], data, size);
        this->position += size;
    }

    inline void WriteU8 (u8  data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteU16(u16 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteU32(u32 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteU64(u64 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteI8 (i8  data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteI16(i16 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteI32(i32 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteI64(i64 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteB8 (b8  data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteF32(f32 data) { this->WriteData(&data, sizeof(data)); }
    inline void WriteF64(f64 data) { this->WriteData(&data, sizeof(data)); }

    template<typename enum_type>
    void WriteEnum(enum_type data) {
        static_assert(std::is_enum_v<enum_type>);
        this->WriteData(&data, sizeof(data));
    }

#if 0
    inline void write_var_int(u64 value)
    {
        for (i32 i = 0; i < sizeof(u64); ++i) {
            bool last = (i == sizeof(u64) - 1);
            u8  part = value & (last ? 0xff : 0x7f);
            value >>= 7;

            this->write_u8((!value ? 0x80 : 0x00) | (part & 0x7f));

            if (!value)
                return;
        }

        assert(!value && "VarInt overflow");
    }
#endif

    inline void WriteString(StringView s) {
        this->WriteU32(static_cast<u32>(s.size()));
        this->WriteData(s.data(), s.size());
    }

    inline bool ReadData(void *buffer, u32 size) {
        if (!this->valid) {
            return false;
        }

        if (static_cast<size_t>(this->position) + size > this->buffer.size()) {
            this->valid = false;
            return false;
        }

        if (size == 0) {
            return true;
        }

        std::memcpy(buffer, &this->buffer[this->position], size);
        this->position += size;

        return true;
    }

    inline bool ReadU8 (u8  &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadU16(u16 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadU32(u32 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadU64(u64 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadI8 (i8  &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadI16(i16 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadI32(i32 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadI64(i64 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadB8 (b8  &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadF32(f32 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }
    inline bool ReadF64(f64 &buffer) { return this->ReadData(&buffer, sizeof(buffer)); }

    template<typename enum_type>
    bool ReadEnum(enum_type &buffer) {
        static_assert(std::is_enum_v<enum_type>);
        return this->ReadData(&buffer, sizeof(buffer));
    }

#if 0
    inline bool read_var_int(u64 &buffer) {
        i32 shift = 0;

        for (i32 i = 0; i < sizeof(u64); ++i) {
            u8 value;

            if (!this->read_u8(value))
                return false;

            bool last = (i == sizeof(u64) - 1);
            *buffer |= (value & (last ? 0xff : 0x7f)) << shift;

            if ((value & 0x80) == 0x00)
                return false;

            shift += 7;
        }

        return true;
    }
#endif

    inline bool ReadString(String &out) {
        if (!this->valid) {
            return false;
        }

        u32 len = 0;

        if (!this->ReadU32(len)) {
            return false;
        }

        if (len > 100000) {
            this->valid = 0;
            return false;
        }

        out.resize(len);

        if (!this->ReadData(out.data(), len)) {
            return false;
        }

        return true;
    }

    inline bool IsValidAndFinished() const {
        return this->valid && this->position == this->buffer.size();
    }

    Array<char> buffer;
    u32 position;
    bool valid;
};
