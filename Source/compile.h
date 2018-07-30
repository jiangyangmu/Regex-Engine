#pragma once

#include "enfa.h"

class FABuilder {
public:
    static v2::ENFA CompileV2(CharArray regex);
};
