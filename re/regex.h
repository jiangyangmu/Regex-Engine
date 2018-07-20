#pragma once

/*
# regex grammar
group = '(' ('?')? (':')? alter ')'
alter = (concat)? ('|' (concat)?)*
concat = item+
item = (char | group) repeat
char = ASCII | '\' [0-9]
repeat = ('*')?
*/

struct Char {
    char c;
};

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
    int min;
    int max;
    bool has_max;
};

//#define DEBUG
#define DEBUG_STATS
