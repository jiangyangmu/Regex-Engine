#pragma once

#include "StringView.h"

#include <string>

/*
# regex grammar
group = '(' ('?')? (':')? alter ')'
alter = (concat)? ('|' (concat)?)*
concat = item+
item = (char | group) repeat
char = ASCII | '\' [0-9]
repeat = ('*')?
*/

template <typename CharT>
struct RegexType {
    typedef CharT Char;
    typedef std::basic_string<CharT> String;
    typedef StringView<CharT> StringView;
};

typedef RegexType<wchar_t>::Char RChar;
typedef RegexType<wchar_t>::String RString;
typedef RegexType<wchar_t>::StringView RView;

struct Group {
    enum Type {
        CAPTURE,
        NON_CAPTURE,
        LOOK_AHEAD,
        LOOK_BEHIND,
        ATOMIC,
    } type;
    union {
        size_t capture_id;
        int lookaround_id;
        int atomic_id;
    };
};

struct BackReference {
    int capture_id;
};

struct Repeat {
    int repeat_id;
    size_t min;
    size_t max;
    bool has_max;
    enum Qualifier {
        GREEDY,
        RELUCTANT
    } qualifier;
};

//#define DEBUG
//#define DEBUG_STATS
