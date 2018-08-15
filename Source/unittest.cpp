#include "stdafx.h"

#include "RegexCompiler.h"

void TrueFalse(RString regex, RString input, bool match) {
    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);
    MatchResult m = enfa.Match(input);
    if (m.matched() != match)
    {
        std::wcout << L"[ERROR] Regex '" << regex << L"' should"
                   << (match ? L" MATCH" : L" NOT MATCH") << L" '" << input
                   << L"'" << std::endl;
    }
}

void RUN_HARDCORE_TEST() {
    // backtracking
    for (int i = 10; i <= 100; i *= 10)
    {
        std::wstring regex(i + 4, 'a');
        regex[0] = '(';
        regex[2] = '*';
        regex[regex.size() - 2] = 'b';
        regex[regex.size() - 1] = ')';
        std::wstring input(i, 'a');
        input.push_back('b');
        TrueFalse(regex, input, true);
    }
}

void RUN_ALL_TEST() {
    TrueFalse(L"(a)", L"a", true);
    TrueFalse(L"(a)", L"b", false);

    TrueFalse(L"(abc)", L"abc", true);
    TrueFalse(L"(abc)", L"ac", false);

    TrueFalse(L"(a|b)", L"a", true);
    TrueFalse(L"(a|b)", L"b", true);
    TrueFalse(L"(a|b)", L"c", false);

    TrueFalse(L"(a*)", L"", true);
    TrueFalse(L"(a*)", L"a", true);
    TrueFalse(L"(a*)", L"aa", true);

    TrueFalse(L"((a)\\1)", L"aa", true);

    TrueFalse(L"((a)(?:b)(c)\\2)", L"abcc", true);
    TrueFalse(L"((a)(?:b)(c)\\2)", L"abca", false);

    TrueFalse(L"((?=y)yes)", L"yes", true);

    TrueFalse(L"(yes(?<s))", L"yes", true);
    TrueFalse(L"(yes(?<yes)(?<es)(?<s))", L"yes", true);
    TrueFalse(L"(yes(?<yes(?<es(?<s))))", L"yes", true);

    TrueFalse(L"(yes(?<y(?=es)e(?=s)s))", L"yes", true);

    TrueFalse(L"((a*)ab)", L"ab", true);
    TrueFalse(L"((a*)ab)", L"aab", true);
    TrueFalse(L"((a*)ab)", L"aaab", true);
    TrueFalse(L"((?>a*)ab)", L"ab", false);
    TrueFalse(L"((?>a*)ab)", L"aab", false);
    TrueFalse(L"((?>a*)ab)", L"aaab", false);

    TrueFalse(L"(a{1,2}b)", L"b", false);
    TrueFalse(L"(a{1,2}b)", L"ab", true);
    TrueFalse(L"(a{1,2}b)", L"aab", true);
    TrueFalse(L"(a{1,2}b)", L"aaab", false);
    TrueFalse(L"(a{3,}b)", L"aab", false);
    TrueFalse(L"(a{3,}b)", L"aaab", true);
    TrueFalse(L"(a{,4}b)", L"aaaab", true);
    TrueFalse(L"(a{,4}b)", L"aaaaab", false);
}