#pragma once

#include <cassert>

template <class T>
inline bool IsDigit(T c) {
    return c >= '0' && c <= '9';
}

template <class T>
int ParseInt32(const T * begin, const T * end) {
    int i = 0;
    while (begin != end)
    {
        assert(IsDigit(*begin));
        i = i * 10 + (*begin - '0');
        ++begin;
    }
    return i;
}

template <class T>
int ParseInt32(const T ** pbegin) {
    const T * begin = *pbegin;
    const T * end = begin;
    while (IsDigit(*end))
        ++end;
    int i = ParseInt32(begin, end);
    *pbegin = end;
    return i;
}
