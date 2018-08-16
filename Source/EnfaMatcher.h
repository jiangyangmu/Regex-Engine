#pragma once

#include "Enfa.h"
#include "StringView.h"
#include "MatchResult.h"

class EnfaMatcher {
    friend class RegexCompiler;

public:
    MatchResult Match(RView text) const;
    std::vector<MatchResult> MatchAll(RView text) const;

private:
    EnfaState * start_;
};
