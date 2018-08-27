#include "stdafx.h"

#include "RegexCompiler.h"

#include <numeric>

static int error_count = 0;

RString CaptureInfoString(const Capture & c) {
    RString s;
    for (auto g = c.GroupBegin(); g != c.GroupEnd(); ++g)
    {
        s += std::to_wstring(g->first) + L":";
        for (const auto & range : g->second.captured())
        {
            s += L" \"";
            s += c.Origin().substr(range.first, range.second - range.first);
            s += L"\"";
        }
        s += L"\n";
    }
    return s;
}

RString CaptureInfoString(const std::map<size_t, std::vector<RString>> & c) {
    RString s;
    for (auto g = c.begin(); g != c.end(); ++g)
    {
        s += std::to_wstring(g->first) + L":";
        for (const auto & text : g->second)
        {
            s += L" \"";
            s += text;
            s += L"\"";
        }
        s += L"\n";
    }
    return s;
}

RString MatchAllInfoString(const std::vector<MatchResult> & matches) {
    RString s;
    if (!matches.empty())
    {
        auto m = matches.begin();
        auto text = m->GetCapture().Group(0).GetLast();
        s += L'"';
        s.append(text.begin(), text.end());
        s += L'"';

        while (++m != matches.end())
        {
            auto text = m->GetCapture().Group(0).GetLast();
            s += L" ";
            s += L'"';
            s.append(text.begin(), text.end());
            s += L'"';
        }
    }
    return s;
}

RString MatchAllInfoString(const std::vector<RString> & matches) {
    RString s;
    if (!matches.empty())
    {
        auto m = matches.begin();
        s += L'"';
        s.append(m->begin(), m->end());
        s += L'"';

        while (++m != matches.end())
        {
            s += L" ";
            s += L'"';
            s.append(m->begin(), m->end());
            s += L'"';
        }
    }
    return s;
}

void TrueFalse(RString regex, RString input, bool match) {
    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);
    MatchResult m = enfa.Match(input);
    if (m.Matched() != match)
    {
        std::wcout << L"[ERROR] Regex '" << regex << L"' should"
                   << (match ? L" MATCH" : L" NOT MATCH") << L" '" << input
                   << L"'" << std::endl;
        ++error_count;
    }
}

void AllMatches(RString regex,
                RString input,
                std::vector<RString> expect_matches) {
    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);
    std::vector<MatchResult> actual_matches = enfa.MatchAll(input);
    RString actual = MatchAllInfoString(actual_matches);
    RString expected = MatchAllInfoString(expect_matches);
    if (actual != expected)
    {
        std::wcout << L"[ERROR] Incorrect matches for regex '" << regex
                   << L"' and text '" << input << "':" << std::endl
                   << L"Expect: " << expected << std::endl
                   << L"Actual: " << actual << std::endl;
        ++error_count;
    }
}

void TrueFalseCapture(RString regex,
                      RString input,
                      bool match,
                      std::map<size_t, std::vector<RString>> captured) {
    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);
    MatchResult m = enfa.Match(input);
    if (m.Matched() != match)
    {
        std::wcout << L"[ERROR] Regex '" << regex << L"' should"
                   << (match ? L" MATCH" : L" NOT MATCH") << L" '" << input
                   << L"'" << std::endl;
        ++error_count;
    }
    else if (m.Matched())
    {
        RString actual = CaptureInfoString(m.GetCapture());
        RString expected = CaptureInfoString(captured);
        if (actual != expected)
        {
            std::wcout << L"[ERROR] Incorrect capture for regex '" << regex
                       << L"' and text '" << input << "':" << std::endl
                       << L"Expect:" << std::endl
                       << expected << L"Actual:" << std::endl
                       << actual;
            ++error_count;
        }
    }
}

// evil case: "(a\0)" ["a", "aa", "aaa", ...]
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
    TrueFalse(L"(a)", L"ab", false);

    // concat
    TrueFalse(L"(abc)", L"abc", true);
    TrueFalse(L"(abc)", L"ac", false);

    // alter
    TrueFalse(L"(a|b)", L"a", true);
    TrueFalse(L"(a|b)", L"b", true);
    TrueFalse(L"(a|b)", L"c", false);
    // alter: backtracking
    TrueFalse(L"((ab|a)b)", L"ab", true);
    // alter: try in order (dependents on group capture)
    TrueFalseCapture(L"((a|aa)a*)", L"aaa", true, {{0, {L"aaa"}}, {1, {L"a"}}});

    // repeat
    TrueFalse(L"(a*)", L"", true);
    TrueFalse(L"(a*)", L"a", true);
    TrueFalse(L"(a*)", L"aa", true);
    // repeat: backtracking
    TrueFalse(L"((a*)ab)", L"ab", true);
    TrueFalse(L"((a*)ab)", L"aab", true);
    TrueFalse(L"((a*)ab)", L"aaab", true);
    // repeat: range
    TrueFalse(L"(a{2}b)", L"ab", false);
    TrueFalse(L"(a{2}b)", L"aab", true);
    TrueFalse(L"(a{2}b)", L"aaab", false);
    TrueFalse(L"(a{1,2}b)", L"b", false);
    TrueFalse(L"(a{1,2}b)", L"ab", true);
    TrueFalse(L"(a{1,2}b)", L"aab", true);
    TrueFalse(L"(a{1,2}b)", L"aaab", false);
    TrueFalse(L"(a{3,}b)", L"aab", false);
    TrueFalse(L"(a{3,}b)", L"aaab", true);
    TrueFalse(L"(a{,4}b)", L"aaaab", true);
    TrueFalse(L"(a{,4}b)", L"aaaaab", false);
    // repeat: greedy vs reluctant
    AllMatches(L"(a*)", L"aaa", {L"aaa", L""});
    AllMatches(L"(a*?)", L"aaa", {L"", L"", L"", L""});
    AllMatches(L"(a{1,2})", L"aa", {L"aa"});
    AllMatches(L"(a{1,2}?)", L"aa", {L"a", L"a"});

    // group: capture
    TrueFalseCapture(L"(a)", L"a", true, {{0, {L"a"}}});
    // group: capture nesting
    TrueFalseCapture(L"(a(b))", L"ab", true, {{0, {L"ab"}}, {1, {L"b"}}});
    TrueFalseCapture(
        L"(a(b(c)))", L"abc", true, {{0, {L"abc"}}, {1, {L"bc"}}, {2, {L"c"}}});
    // group: capture back reference
    TrueFalse(L"((a)\\1)", L"aa", true);

    // group: non-capture
    TrueFalseCapture(L"(a(?:b))", L"ab", true, {{0, {L"ab"}}});

    // group: capture back reference + non-capture
    TrueFalse(L"((a)(?:b)(c)\\2)", L"abcc", true);
    TrueFalse(L"((a)(?:b)(c)\\2)", L"abcb", false);

    // group: look-ahead assertion
    TrueFalse(L"((?=y)yes)", L"yes", true);
    // group: look-behind assertion
    TrueFalse(L"(yes(?<s))", L"yes", true);
    // group: look-around assertion nesting
    TrueFalse(L"(yes(?<yes)(?<es)(?<s))", L"yes", true);
    TrueFalse(L"(yes(?<yes(?<es(?<s))))", L"yes", true);
    TrueFalse(L"(yes(?<y(?=es)e(?=s)s))", L"yes", true);

    // group: atomic group (disable backtracking)
    TrueFalse(L"((?>a*)b)", L"ab", true);
    TrueFalse(L"((?>a*)ab)", L"ab", false);
    TrueFalse(L"((?>a*)ab)", L"aab", false);
    TrueFalse(L"((?>a|ab)b)", L"ab", true);
    TrueFalse(L"((?>a|ab)b)", L"abb", false);
}

//#include <codecvt>
//#include <fstream>
//#include <locale>
// void FILE_TEST() {
//    std::wifstream input(
//        "C:/Users/celsi/Desktop/Projects/re/Build/x64/Debug/input.utf16.html",
//        std::ios::binary);
//    input.imbue(std::locale(
//        input.getloc(),
//        new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
//
//    RString regex;
//    input >> regex;
//    std::wcout << L"pattern: " << regex << std::endl;
//    EnfaMatcher enfa = RegexCompiler::CompileToEnfa(regex);
//    for (RString line; std::getline(input, line);)
//    {
//        int i = 0;
//        auto matches = enfa.MatchAll(line);
//        if (matches.empty())
//            std::wcout << "No matches." << std::endl;
//        for (MatchResult & m : matches)
//        {
//            std::wcout << "Match Result " << i++ << std::endl;
//            if (m.Matched())
//                std::wcout << m.GetCapture().DebugString() << std::endl;
//        }
//    }
//}

int main(int argc, char * argv[]) {
    RUN_ALL_TEST();
    if (error_count == 0)
    {
        std::wcout << "All unittest passed." << std::endl;
    }
    return 0;
}
