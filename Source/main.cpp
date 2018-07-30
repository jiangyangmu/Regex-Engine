#include "stdafx.h"

#include "compile.h"

#include <fcntl.h>
#include <io.h>

// evil case: "(a\0)" ["a", "aa", "aaa", ...]
void RUN_ALL_TEST();
void RUN_HARDCORE_TEST();

int main() {
    // RUN_ALL_TEST();
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

    // std::string regex = "((?>a*)ab)";

    // Fix console output.
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    CharArray regex = L"((ab{1,2}){1,3})";
    //std::wcin >> regex;

    // v1::ENFA enfa = FABuilder::CompileV1(regex);
    v2::ENFA enfa = FABuilder::CompileV2(regex);

    std::wcout << L"pattern: " << regex << std::endl;
    for (CharArray line; std::getline(std::wcin, line);)
    {
        // MatchResult m1 = enfa.Match(line);
        // if (m1.matched())
        //    std::wcout << m1.capture().DebugString() << std::endl;
        // std::wcout << L"ENFA: " << (m1.matched() ? L"ok." : L"reject.")
        //           << std::endl;
        int i = 0;
        for (MatchResult & m : enfa.MatchAll(line))
        {
            std::wcout << "Match Result " << i++ << std::endl;
            if (m.matched())
                std::wcout << m.capture().DebugString() << std::endl;
        }
    }

    return 0;
}
