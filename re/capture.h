#pragma once

#include "regex.h"

class Capture;

namespace v2 {

struct CaptureTag {
    size_t capture_id;
    bool is_begin;
};

} // namespace v2

class CaptureGroup {
public:
    CaptureGroup()
        : need_begin_(true) {
    }

    std::pair<size_t, size_t> Last() const {
        assert(IsComplete());
        return captured_.back();
    }
    bool IsComplete() const {
        return need_begin_ && !captured_.empty();
    }

    void Begin(size_t pos) {
        if (need_begin_)
        {
            captured_.push_back({pos, 0});
            need_begin_ = false;
        }
        events_.emplace_back(true, pos);
    }
    void End(size_t pos) {
        if (!need_begin_)
        {
            captured_.back().second = pos;
            need_begin_ = true;
        }
        events_.emplace_back(false, pos);
    }

    const std::vector<std::pair<size_t, size_t>> & captured() const {
        return captured_;
    }
    const std::vector<std::pair<bool, size_t>> & events() const {
        return events_;
    }

private:
    bool need_begin_;
    std::vector<std::pair<size_t, size_t>> captured_;
    std::vector<std::pair<bool, size_t>> events_;
};

class Capture {
public:
    Capture(const CharArray & origin)
        : origin_(origin) {
    }

    void CopyTo(Capture & c) const {
        c.~Capture();
        ::new (&c) Capture(origin_);
        c.capture_groups_ = capture_groups_;
    }
    const CaptureGroup & Group(size_t group_id) {
        assert(group_id < capture_groups_.size());
        return capture_groups_[group_id];
    }

    void DoCapture(const v2::CaptureTag & tag, size_t pos) {
        if (tag.is_begin)
            capture_groups_[tag.capture_id].Begin(pos);
        else
            capture_groups_[tag.capture_id].End(pos);
    }

    const CharArray & origin() const {
        return origin_;
    }

    CharArray DebugString() const {
        CharArray s;
        for (const auto & kv : capture_groups_)
        {
            s += std::to_wstring(kv.first) + L":";
            for (const auto & range : kv.second.captured())
            {
                s += L" \"";
                s += origin_.substr(range.first, range.second - range.first);
                s += L"\"";
            }
            s += L"\n";
        }
        for (const auto & kv : capture_groups_)
        {
            s += std::to_wstring(kv.first) + L":";
            for (const auto & p : kv.second.events())
            {
                s += ' ';
                s += (p.first ? 'B' : 'E');
                s += ':';
                s += std::to_wstring(p.second);
            }
            s += L"\n";
        }
        return s;
    }

private:
    const CharArray & origin_;
    std::map<size_t, CaptureGroup> capture_groups_;
};

class MatchResult {
public:
    MatchResult(const Capture & capture, bool matched)
        : capture_(capture)
        , matched_(matched) {
    }
    bool matched() const {
        return matched_;
    }
    const Capture & capture() const {
        return capture_;
    }

private:
    bool matched_;
    Capture capture_;
};
