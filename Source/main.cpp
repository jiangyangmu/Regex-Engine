#include "stdafx.h"

#include "Postfix.h"
#include "RegexCompiler.h"

#include <fcntl.h>
#include <io.h>

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

    // std::string regex = "((?>a*)ab)";

    // Fix console output.
    _setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    CharArray regex = L"(yes(?<(?=yes)y(?<y)(?=es)e(?=s)s(?<s)))";
    // CharArray regex = L"((ab{1,2}){1,3})";
    // std::wcin >> regex;

    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);

    for (CharArray line; std::wcout << L"pattern: " << regex << std::endl,
                         std::getline(std::wcin, line);)
    {
        int i = 0;
        auto matches = enfa.MatchAll(line);
        if (matches.empty())
            std::wcout << "No matches." << std::endl;
        for (MatchResult & m : matches)
        {
            std::wcout << "Match Result " << i++ << std::endl;
            if (m.matched())
                std::wcout << m.capture().DebugString() << std::endl;
        }
    }

    return 0;
}
