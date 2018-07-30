#pragma once

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

struct CaptureTag {
    size_t capture_id;
    bool is_begin;
};

struct LookAroundTag {
    int id;
    bool is_begin;
    bool is_forward;
};

class TagSet {
public:
    TagSet()
        : has_capture_(false)
        , has_lookaround_(false)
        , has_atomic_(false)
        , has_repeat_(false) {
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
        has_repeat_ = true;
        repeat_ = tag;
    }

private:
    bool has_capture_;
    CaptureTag capture_;
    bool has_lookaround_;
    LookAroundTag lookaround_;
    bool has_atomic_;
    AtomicTag atomic_;
    bool has_repeat_;
    RepeatTag repeat_;
};
