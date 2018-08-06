#pragma once

#include "stdafx.h"

#include "EnfaTag.h"
#include "RegexSyntax.h"
#include "capture.h"

class EnfaState {
    friend class EnfaStateBuilder;

public:
    bool IsChar() const {
        return edge.type_ == CHAR_OUT;
    }
    bool IsBackReference() const {
        return edge.type_ == BACKREF_OUT;
    }
    bool IsEpsilon() const {
        return edge.type_ == EPSILON_OUT;
    }
    bool IsFinal() const {
        return is_final_;
    }

    ::Char::Type Char() const {
        assert(IsChar());
        return edge.char_.c;
    }
    size_t BackReference() const {
        assert(IsBackReference());
        return edge.backref_;
    }

    // char, back reference
    const EnfaState * Out() const {
        assert((IsChar() || IsBackReference() || IsEpsilon()) &&
               edge.out_.size() == 1);
        return edge.out_.front();
    }
    // epsilon
    const std::vector<EnfaState *> & MultipleOut() const {
        assert(IsEpsilon() && !edge.out_.empty());
        return edge.out_;
    }
    const std::vector<EnfaState *> & DebugMultipleOut() const {
        return edge.out_;
    }
    void SetFinal() {
        is_final_ = true;
    }

    const TagSet & Tags() const {
        return tag_set_;
    }
    TagSet & Tags() {
        return tag_set_;
    }

    static CharArray DebugString(EnfaState * start);

private:
    EnfaState() {
        edge = {
            INVALID,
            0,
            0,
        };
        is_final_ = false;
    }

    enum Type {
        INVALID,
        CHAR_OUT,
        BACKREF_OUT,
        EPSILON_OUT,
    };

    struct Edge {
        Type type_;
        ::Char char_;
        size_t backref_;
        std::vector<EnfaState *> out_;
    } edge;

    bool is_final_;

    TagSet tag_set_;
};

class EnfaStateBuilder {
public:
    struct StatePort {
        EnfaState * in;
        EnfaState * out;
    };
    static StatePort Char(::Char::Type c);
    static StatePort BackReference(size_t ref_capture_id);
    static StatePort Alter(StatePort sp1, StatePort sp2);
    static StatePort Repeat(StatePort sp, struct Repeat rep);
    static StatePort Concat(StatePort sp1, StatePort sp2);
    static StatePort Group(StatePort sp);
};
