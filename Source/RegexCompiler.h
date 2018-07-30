#pragma once

#include "Enfa.h"

class RegexCompiler {
public:
    static EnfaMatcher CompileToEnfa(CharArray regex);
};
