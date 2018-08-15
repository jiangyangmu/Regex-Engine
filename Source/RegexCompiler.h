#pragma once

#include "EnfaMatcher.h"

class RegexCompiler {
public:
    static EnfaMatcher CompileToEnfa(RString regex);
};
