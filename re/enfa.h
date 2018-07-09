#pragma once

#include "stdafx.h"

#include "capture.h"

class EnfaState {
    friend class EnfaStateBuilder;

public:
    bool IsChar() const {
        return type_ >= CHAR_MIN_ && type_ <= CHAR_MAX_;
    }
    bool IsBackReference() const {
        return type_ >= BACKREF_0_ && type_ <= BACKREF_9_;
    }
    bool IsSplit() const {
        return type_ == SPLIT;
    }
    bool IsFinal() const {
        return type_ == FINAL;
    }

    char Char() const {
        assert(IsChar());
        return static_cast<char>(type_);
    }
    size_t BackReference() const {
        assert(IsBackReference());
        return type_ - BACKREF_0_;
    }
    const EnfaState * Out() const {
        assert(!IsFinal());
        return out_;
    }
    const EnfaState * Out1() const {
        assert(!IsFinal());
        return out1_;
    }
    const CaptureTag & Tag() const {
        return tag_;
    }

    EnfaState ** MutableOut() {
        assert(!IsFinal());
        return &out_;
    }
    EnfaState ** MutableOut1() {
        assert(!IsFinal());
        return &out1_;
    }
    CaptureTag * MutableTag() {
        return &tag_;
    }

    static std::string DebugString(EnfaState * start);

private:
    enum {
        CHAR_MIN_ = 0,
        CHAR_MAX_ = 255,
        SPLIT = 256,
        FINAL = 257,
        BACKREF_0_ = 258,
        BACKREF_1_ = 259,
        BACKREF_2_ = 260,
        BACKREF_3_ = 261,
        BACKREF_4_ = 262,
        BACKREF_5_ = 263,
        BACKREF_6_ = 264,
        BACKREF_7_ = 265,
        BACKREF_8_ = 266,
        BACKREF_9_ = 267,
    };

    int type_;
    EnfaState * out_;
    EnfaState * out1_;

    CaptureTag tag_;
};

class EnfaStateBuilder {
public:
    static EnfaState * NewCharState(char c);
    static EnfaState * NewBackReferenceState(size_t ref_group_id);
    static EnfaState * NewSplitState(EnfaState * out, EnfaState * out1);
    static EnfaState * NewFinalState();
};

class ENFA {
    friend class FABuilder;

public:
    MatchResult Match(const std::string & text) const;

private:
    EnfaState * start_;
};
