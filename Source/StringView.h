#pragma once

#include <ostream>
#include <string>
#include <cassert>

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
    typedef const CharType * iterator;
    typedef const CharType * const_iterator;

    StringView(const StringType & str)
        : begin_(str.data())
        , end_(str.data() + str.size()) {
    }
    StringView(const CharType * data)
        : begin_(data)
        , end_(data) {
        while (*end_)
            ++end_;
    }
    explicit StringView(const CharType * data, size_t count)
        : begin_(data)
        , end_(data + count) {
    }

    void popFront(size_t count) {
        begin_ = (size() > count ? begin_ + count : end_);
    }

    void popBack(size_t count) {
        end_ = (size() > count ? end_ - count : begin_);
    }

    const CharType * data() const {
        return begin_;
    }

    bool empty() const {
        return end_ == begin_;
    }

    size_t size() const {
        assert(end_ >= begin_);
        return static_cast<size_t>(end_ - begin_);
    }

    StringType substr(size_t offset, size_t count) const {
        offset = (offset > size() ? size() : offset);
        count = (count > (size() - offset) ? (size() - offset) : count);
        return StringType(begin_ + offset, count);
    }

    CharType operator[](size_t index) const {
        assert(index < size());
        return *(begin_ + index);
    }

    iterator begin() const {
        return begin_;
    }

    iterator end() const {
        return end_;
    }

    friend OStreamType & operator<<(OStreamType & o, const StringView<T> & sv) {
        for (const CharType * c = sv.begin_; c != sv.end_; ++c)
            o << *c;
        return o;
    }

private:
    const CharType * begin_;
    const CharType * end_;
};
