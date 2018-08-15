#pragma once

#include "RegexSyntax.h"

#include <cassert>

class CharMatcher {
public:
    explicit CharMatcher(RView source);

    RView Origin() const;
    size_t CurrentPos() const;

    RChar GetChar();

    bool Match(RChar c);
    bool TryMatch(RChar c) const;
    bool MatchTwo(RChar c1, RChar c2);
    bool MatchRange(RView range);
    bool TryMatchAny(RView candidates) const;

    // TODO: rename to BackwordXXX
    bool MatchBackword(RChar c);
    bool MatchRangeBackword(RView range);

protected:
    RView source_;
    size_t pos_;
};

class LexMatcher : public CharMatcher {
public:
    explicit LexMatcher(RView source);
    bool MatchUInt32(size_t * value);
    bool TryMatchUInt32() const;
};
