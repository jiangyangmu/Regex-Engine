#include "stdafx.h"

#include "Postfix.h"
#include "RegexCompiler.h"

#include <fcntl.h>
#include <io.h>

int main(int argc, char * argv[]) {
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
    //_setmode(_fileno(stdin), _O_U16TEXT);
    _setmode(_fileno(stdout), _O_U16TEXT);

    //RString regex = L"(yes(?<(?=yes)y(?<y)(?=es)e(?=s)s(?<s)))";
     //RString regex = L"((ab{1,2}){1,3})";
    RString regex = L"(a{1,2}b)";
     
    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);

    for (RString line; std::wcout << L"pattern: " << regex << std::endl,
                       std::getline(std::wcin, line);)
    {
        int i = 0;
        auto matches = enfa.MatchAll(line);
        if (matches.empty())
            std::wcout << "No matches." << std::endl;
        for (MatchResult & m : matches)
        {
            std::wcout << "Match Result " << i++ << std::endl;
            if (m.Matched())
                std::wcout << m.GetCapture().DebugString() << std::endl;
        }
    }

    return 0;
}
