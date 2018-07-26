#pragma once

#include "stdafx.h"

#include "capture.h"
#include "lookaround.h"
#include "regex.h"

class FABuilder;

namespace v2 {

struct AtomicTag {
    int atomic_id;
    bool is_begin;
};

struct RepeatTag {
    int repeat_id;
    size_t min;
    size_t max;
    bool has_max;
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
    bool HasRepeatTag() const {
        return has_repeat_;
    }
    const RepeatTag & GetRepeatTag() const {
        return repeat_;
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
    void SetRepeatTag(RepeatTag tag) {
        has_repeat_ =  true;
        repeat_ = tag;
    }

    void SetForwardMode() const {
        direction_ = &forward;
    }
    void SetBackwordMode() const {
        direction_ = &backword;
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
    bool has_repeat_;
    RepeatTag repeat_;
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

class ENFA {
    friend class FABuilder;

public:
    MatchResult Match(const CharArray & text) const;

private:
    EnfaState * start_;
};

} // namespace v2
