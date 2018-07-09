#pragma once

#include "enfa.h"

class FABuilder {
public:
    static ENFA Compile(std::string regex);
};
