#pragma once

#include "stdafx.h"
#include "IntType.h"

class ByteStream {
public:
    explicit ByteStream(const char *filename);
    explicit ByteStream(const std::string & str);
    explicit ByteStream(const std::istream & in);

    BYTE PeekByte(size_t offset);
};

// handle encoding
class WordStream {
public:

};

std::unique_ptr<WordStream> CreateWordStream(ByteStream & bs);
