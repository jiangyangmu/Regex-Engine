#pragma once

#include "stdafx.h"

#include "capture.h"
#include "lookaround.h"
#include "regex.h"

class FABuilder;

namespace v1 {

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

} // namespace v1

namespace v2 {

struct AtomicTag {
    int atomic_id;
    bool is_begin;
};

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

    char Char() const {
        assert(IsChar());
        return direction_->char_.c;
    }
    size_t BackReference() const {
        assert(IsBackReference());
        return direction_->backref_;
    }

    // char, back reference
    const EnfaState * Out() const {
        assert((IsChar() || IsBackReference() || IsEpsilon()) && direction_->out_.size() == 1);
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

    bool HasCaptureTag() const {
        return has_capture_;
    }
    const CaptureTag & GetCaptureTag() const {
        return capture_;
    }
    bool HasLookAroundTag() const {
        return has_lookaround_;
    }
    const LookAroundTag & GetLookAroundTag() const {
        return lookaround_;
    }
    bool HasAtomicTag() const {
        return has_atomic_;
    }
    const AtomicTag & GetAtomicTag() const {
        return atomic_;
    }

    void SetFinal() {
        is_final_ = true;
    }
    void SetCaptureTag(CaptureTag tag) {
        has_capture_ = true;
        capture_ = tag;
    }
    void SetLookAroundTag(LookAroundTag tag) {
        has_lookaround_ = true;
        lookaround_ = tag;
    }
    void SetAtomicTag(AtomicTag tag) {
        has_atomic_ = true;
        atomic_ = tag;
    }

    void SetForwardMode() const {
        direction_ = &forward;
    }
    void SetBackwordMode() const {
        direction_ = &backword;
    }

    static std::string DebugString(EnfaState * start);

private:
    EnfaState() {
        forward = backword = {
            INVALID,
            0,
            0,
        };
        direction_ = &forward;
        is_final_ = false;
        has_capture_ = false;
        has_lookaround_ = false;
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

    bool has_capture_;
    CaptureTag capture_;
    bool has_lookaround_;
    LookAroundTag lookaround_;
    bool has_atomic_;
    AtomicTag atomic_;
};

class EnfaStateBuilder {
public:
    struct StatePort {
        EnfaState * in;
        EnfaState * out;
    };
    static StatePort Char(char c);
    static StatePort BackReference(size_t ref_capture_id);
    static StatePort Alter(StatePort sp1, StatePort sp2);
    static StatePort Repeat(StatePort sp);
    static StatePort Concat(StatePort sp1, StatePort sp2);
    static StatePort Group(StatePort sp);
    static StatePort InverseGroup(StatePort sp);
};

class ENFA {
    friend class FABuilder;

public:
    MatchResult Match(const std::string & text) const;

private:
    EnfaState * start_;
};

} // namespace v2
