#pragma once

#include "enfa.h"

class RegexCompiler {
public:
    static v2::EnfaMatcher CompileToEnfa(CharArray regex);
};
