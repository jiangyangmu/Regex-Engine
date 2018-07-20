#pragma once

#include <cassert>

inline bool IsDigit(char c) {
    return c >= '0' && c <= '9';
}

int ParseInt32(const char * begin, const char * end) {
    int i = 0;
    while (begin != end)
    {
        assert(IsDigit(*begin));
        i = i * 10 + (*begin - '0');
        ++begin;
    }
    return i;
}

int ParseInt32(const char ** pbegin) {
    const char * begin = *pbegin;
    const char * end = begin;
    while (IsDigit(*end))
        ++end;
    int i = ParseInt32(begin, end);
    *pbegin = end;
    return i;
}
