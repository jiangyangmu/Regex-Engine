#pragma once

#include "stdafx.h"

#include "EnfaTag.h"
#include "RegexSyntax.h"
#include "capture.h"

class RegexCompiler;

class EnfaState {
    friend class EnfaStateBuilder;

public:
    bool IsChar() const {
        return direction_->type_ == CHAR_OUT;
    }
    bool IsBackReference() const {
        return direction_->type_ == BACKREF_OUT;
    }
    bool IsEpsilon() const {
        return direction_->type_ == EPSILON_OUT;
    }
    bool IsFinal() const {
        return is_final_;
    }

    ::Char::Type Char() const {
        assert(IsChar());
        return direction_->char_.c;
    }
    size_t BackReference() const {
        assert(IsBackReference());
        return direction_->backref_;
    }

    // char, back reference
    const EnfaState * Out() const {
        assert((IsChar() || IsBackReference() || IsEpsilon()) &&
               direction_->out_.size() == 1);
        return direction_->out_.front();
    }
    // epsilon
    const std::vector<EnfaState *> & MultipleOut() const {
        assert(IsEpsilon() && !direction_->out_.empty());
        return direction_->out_;
    }
    const std::vector<EnfaState *> & DebugMultipleOut() const {
        return direction_->out_;
    }
    void SetFinal() {
        is_final_ = true;
    }
    void SetForwardMode() const {
        direction_ = &forward;
    }
    void SetBackwordMode() const {
        direction_ = &backword;
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
        forward = backword = {
            INVALID,
            0,
            0,
        };
        direction_ = &forward;
        is_final_ = false;
    }

    enum Type {
        INVALID,
        CHAR_OUT,
        BACKREF_OUT,
        EPSILON_OUT,
    };

    struct Direction {
        Type type_;
        ::Char char_;
        size_t backref_;
        std::vector<EnfaState *> out_;
    } forward, backword;
    mutable const Direction * direction_;

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
    static StatePort InverseGroup(StatePort sp);
};

class EnfaMatcher {
    friend class RegexCompiler;

public:
    MatchResult Match(StringView<wchar_t> text) const;
    std::vector<MatchResult> MatchAll(StringView<wchar_t> text) const;

private:
    EnfaState * start_;
};
