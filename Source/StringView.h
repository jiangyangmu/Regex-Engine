#pragma once

#include <ostream>
#include <string>

template <typename T>
struct TypeTraits;
template <>
struct TypeTraits<char> {
    typedef std::string StringType;
    typedef std::ostream OStreamType;
};
template <>
struct TypeTraits<wchar_t> {
    typedef std::wstring StringType;
    typedef std::wostream OStreamType;
};

template <typename T>
class StringView {
public:
    typedef T CharType;
    typedef typename TypeTraits<T>::StringType StringType;
    typedef typename TypeTraits<T>::OStreamType OStreamType;

    StringView(const StringType & str)
        : begin(str.data())
        , end(str.data() + str.size()) {
    }
    explicit StringView(const CharType * data, size_t count)
        : begin(data)
        , end(data + count) {
    }

    void popFront(size_t count) {
        begin = (size() > count ? begin + count : end);
    }

    void popBack(size_t count) {
        end = (size() > count ? end - count : begin);
    }

    const CharType * data() const {
        return begin;
    }

    bool empty() const {
        return end == begin;
    }

    size_t size() const {
        assert(end >= begin);
        return static_cast<size_t>(end - begin);
    }

    StringType substr(size_t offset, size_t count) const {
        offset = (offset > size() ? size() : offset);
        count = (count > (size() - offset) ? (size() - offset) : count);
        return StringType(begin + offset, count);
    }

    CharType operator[](size_t index) const {
        assert(index < size());
        return *(begin + index);
    }

    friend OStreamType & operator<<(OStreamType & o, const StringView<T> & sv) {
        for (const CharType * c = sv.begin; c != sv.end; ++c)
            o << *c;
        return o;
    }

private:
    const CharType * begin;
    const CharType * end;
};
