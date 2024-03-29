#pragma once

#include "stdafx.h"
#include "RegexSyntax.h"

/*
* regex string to postfix
*/

// postfix node
struct PostfixNode {
    enum Type {
        INVALID,
        NULL_INPUT, // empty group or alternation
        CHAR_INPUT,
        BACKREF_INPUT,
        REPEAT,
        CONCAT,
        ALTER,
        GROUP,
    };

    explicit PostfixNode(RChar c)
        : type(CHAR_INPUT)
        , chr(c) {
    }
    explicit PostfixNode(Group g)
        : type(GROUP)
        , group(g) {
    }
    explicit PostfixNode(BackReference br)
        : type(BACKREF_INPUT)
        , backref(br) {
    }
    explicit PostfixNode(Repeat rep)
        : type(REPEAT)
        , repeat(rep) {
    }
    explicit PostfixNode(Type t)
        : type(t) {
        assert(type == CONCAT || type == ALTER || type == NULL_INPUT);
    }

    RString DebugString() const;

    Type type;
    union {
        RChar chr;
        Group group;
        BackReference backref;
        Repeat repeat;
    };
};

std::vector<PostfixNode> ParseToPostfix(RString regex);
std::vector<PostfixNode> FlipPostfix(const std::vector<PostfixNode> & nl);
