#pragma once

#include "enfa.h"

class FABuilder {
public:
    static v1::ENFA CompileV1(std::string regex);
    static v2::ENFA CompileV2(std::string regex);
};
