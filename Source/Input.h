#pragma once

#include "IntType.h"
#include "stdafx.h"

class ByteArray {};

class UTF8Encoding {
public:
    bool HasNext() const {
        return pos_ < end_;
    }
    UINT32 Next() {
    }

    bool HasPrev() const {
        return pos_ > begin_;
    }
    UINT32 Prev() {
        // TODO: optimize
        assert(HasPrev());
        if ((*--pos_ & 0x80) == 0)
            return *pos_;
        if ((*--pos_ & 0xE0) == 0xC0)
            return (((UINT32)(*pos_ & 0x1F) << 8) |
                    ((UINT32)(*(pos_ + 1) & 0x3F)));
        if ((*--pos_ & 0xF0) == 0xE0)
            return (((UINT32)(*pos_ & 0x0F) << 16) |
                    ((UINT32)(*(pos_ + 1) & 0x3F) << 8) |
                    ((UINT32)(*(pos_ + 2) & 0x3F)));
        --pos_;
        assert((*pos_ & 0xF8) == 0xF0);
        return (((UINT32)(*pos_ & 0x06) << 24) |
                ((UINT32)(*(pos_ + 1) & 0x3F) << 16) |
                ((UINT32)(*(pos_ + 2) & 0x3F) << 8) |
                ((UINT32)(*(pos_ + 3) & 0x3F)));
    }

    size_t CharCount() const {
        return char_count_;
    }
    size_t ByteCount() const {
        return static_cast<size_t>(end_ - begin_);
    }

private:
    const char * begin_;
    const char * end_;
    const char * pos_;
    size_t char_count_;
};

// TODO: UTF16Encoding
// TODO: ASCIIEncoding
// regex engine doesn't care actual character, it only needs "match one" and "match range",
// so for ascii, ASCIIEncoding is best, for unicode, UTF16Encoding is best.

// decoder
// class InputStream {
// public:
//    enum Encoding { UNKNOWN, UTF8, UTF16LE, UTF16BE, UCS2, UTF32LE, UTF32BE };
//
//    Encoding GetEncoding() const;
//
//    bool HasNext();
//    UINT32 Next();
//};
