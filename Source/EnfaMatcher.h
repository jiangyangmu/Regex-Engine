#pragma once

#include "Enfa.h"
#include "StringView.h"
#include "capture.h"

class EnfaMatcher {
    friend class RegexCompiler;

public:
    MatchResult Match(StringView<wchar_t> text) const;
    std::vector<MatchResult> MatchAll(StringView<wchar_t> text) const;

private:
    EnfaState * start_;
};
