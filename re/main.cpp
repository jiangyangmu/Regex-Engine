#include "stdafx.h"

#include "compile.h"

// evil case: "(a\0)" ["a", "aa", "aaa", ...]
void RUN_ALL_TEST();
void RUN_HARDCORE_TEST();

int main() {
    RUN_ALL_TEST();
    // RUN_HARDCORE_TEST();

    // std::string regex = "(ab*|c)";
    // std::string regex = "((a)*)";
    // std::string regex = "((a*)*)";
    // std::string regex = "((a(b*)c)*)";
    // std::string regex = "((a*)|(b*)|(c*))";
    // std::string regex = "((a*)|(a)*|(a*)*)";
    // std::string regex = "((a(b)*c|d(e*)f)*)";
    // std::string regex = "((aa*)|(a*))";

    // std::string regex = "((abc)\\1(?:d)(e)\\2)";
    // std::string regex = "((a*)*)";
    // std::string regex = "((?=y)yes(?<s))";
    // std::string regex = "(a{,}b{1,}c{,2}d{3,4})";

    std::string regex = "((?>a*)ab)";

    // v1::ENFA enfa = FABuilder::CompileV1(regex);
    v2::ENFA enfa = FABuilder::CompileV2(regex);

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        MatchResult m1 = enfa.Match(line);
        if (m1.matched())
            std::cout << m1.capture().DebugString() << std::endl;
        std::cout << "ENFA: " << (m1.matched() ? "ok." : "reject.")
                  << std::endl;
    }

    return 0;
}
