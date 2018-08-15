#include "stdafx.h"

#include "CharMatcher.h"
#include "util.h"

CharMatcher::CharMatcher(RView source)
    : source_(source)
    , pos_(0) {
}

RView CharMatcher::Origin() const {
    return source_;
}

size_t CharMatcher::CurrentPos() const {
    return pos_;
}

RChar CharMatcher::GetChar() {
    assert(pos_ < source_.size());
    return source_[pos_++];
}

bool CharMatcher::Match(RChar c) {
    if (pos_ < source_.size() && source_[pos_] == c)
    {
        ++pos_;
        return true;
    }
    else
        return false;
}

bool CharMatcher::TryMatch(RChar c) const {
    return pos_ < source_.size() && source_[pos_] == c;
}

bool CharMatcher::MatchTwo(RChar c1, RChar c2) {
    if (pos_ + 1 < source_.size() && source_[pos_] == c1 &&
        source_[pos_ + 1] == c2)
    {
        pos_ += 2;
        return true;
    }
    else
        return false;
}

bool CharMatcher::MatchRange(RView range) {
    if (pos_ + range.size() > source_.size())
        return false;
    size_t p = pos_;
    for (auto c : range)
    {
        if (source_[p++] != c)
            return false;
    }
    pos_ = p;
    return true;
}

bool CharMatcher::TryMatchAny(RView candidates) const {
    if (pos_ < source_.size())
    {
        RChar c = source_[pos_];
        for (RChar ch : candidates)
        {
            if (ch == c)
                return true;
        }
    }
    return false;
}

bool CharMatcher::MatchBackword(RChar c) {
    if (pos_ > 0 && source_[pos_ - 1] == c)
    {
        --pos_;
        return true;
    }
    else
        return false;
}

bool CharMatcher::MatchRangeBackword(RView range) {
    if (pos_ < range.size())
        return false;
    size_t p = pos_;
    for (auto c : range)
    {
        if (source_[--p] != c)
            return false;
    }
    pos_ = p;
    return true;
}

LexMatcher::LexMatcher(RView source)
    : CharMatcher(source) {
}

bool LexMatcher::MatchUInt32(size_t * value) {
    const RChar *pbegin, *pend;
    pbegin = pend = source_.data() + pos_;

    int ival = ParseInt32(&pend);
    if (pend > pbegin && ival >= 0)
    {
        *value = (size_t)ival;
        pos_ += pend - pbegin;
        return true;
    }
    else
        return false;
}

bool LexMatcher::TryMatchUInt32() const {
    const RChar *pbegin, *pend;
    pbegin = pend = source_.data() + pos_;

    int ival = ParseInt32(&pend);
    return (pend > pbegin && ival >= 0);
}
