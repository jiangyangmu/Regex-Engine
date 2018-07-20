#include "stdafx.h"

#include "compile.h"

void TrueFalse(std::string regex, std::string input, bool match) {
    v2::ENFA enfa = FABuilder::CompileV2(regex);
    MatchResult m = enfa.Match(input);
    if (m.matched() != match)
    {
        std::cout << "[ERROR] Regex '" << regex << "' should"
                  << (match ? " MATCH" : " NOT MATCH") << " '" << input << "'"
                  << std::endl;
    }
}

void RUN_HARDCORE_TEST() {
    // backtracking
    for (int i = 10; i <= 100; i *= 10)
    {
        std::string regex(i + 4, 'a');
        regex[0] = '(';
        regex[2] = '*';
        regex[regex.size() - 2] = 'b';
        regex[regex.size() - 1] = ')';
        std::string input(i, 'a');
        input.push_back('b');
        TrueFalse(regex, input, true);
    }
}

void RUN_ALL_TEST() {
    TrueFalse("(a)", "a", true);
    TrueFalse("(a)", "b", false);

    TrueFalse("(abc)", "abc", true);
    TrueFalse("(abc)", "ac", false);

    TrueFalse("(a|b)", "a", true);
    TrueFalse("(a|b)", "b", true);
    TrueFalse("(a|b)", "c", false);

    TrueFalse("(a*)", "", true);
    TrueFalse("(a*)", "a", true);
    TrueFalse("(a*)", "aa", true);

    TrueFalse("((a)\\1)", "aa", true);

    TrueFalse("((a)(?:b)(c)\\2)", "abcc", true);
    TrueFalse("((a)(?:b)(c)\\2)", "abca", false);

    TrueFalse("((?=y)yes)", "yes", true);

    TrueFalse("(yes(?<s))", "yes", true);

    TrueFalse("((a*)ab)", "ab", true);
    TrueFalse("((a*)ab)", "aab", true);
    TrueFalse("((a*)ab)", "aaab", true);
    TrueFalse("((?>a*)ab)", "ab", false);
    TrueFalse("((?>a*)ab)", "aab", false);
    TrueFalse("((?>a*)ab)", "aaab", false);
}