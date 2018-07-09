#pragma once

#include "stdafx.h"

#include "capture.h"

class EnfaState {
    friend class EnfaStateBuilder;
    friend class ENFA;

public:
    bool IsChar() const {
        return c >= CHAR_MIN_ && c <= CHAR_MAX_;
    }
    bool IsSplit() const {
        return c == SPLIT;
    }
    bool IsFinal() const {
        return c == FINAL;
    }

    char Char() const {
        assert(IsChar());
        return static_cast<char>(c);
    }
    const EnfaState * Out() const {
        assert(!IsFinal());
        return out;
    }
    const EnfaState * Out1() const {
        assert(!IsFinal());
        return out1;
    }
    const CaptureTag & Tag() const {
        return tag_;
    }

    EnfaState ** MutableOut() {
        assert(!IsFinal());
        return &out;
    }
    EnfaState ** MutableOut1() {
        assert(!IsFinal());
        return &out1;
    }
    CaptureTag * MutableTag() {
        return &tag_;
    }

    static std::string DebugString(EnfaState * start);

private:
    enum Type { CHAR_MIN_ = 0, CHAR_MAX_ = 255, SPLIT = 256, FINAL = 257 };

    int c;
    EnfaState * out;
    EnfaState * out1;

    CaptureTag tag_;
};

class EnfaStateBuilder {
public:
    static EnfaState * NewCharState(char c);
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
