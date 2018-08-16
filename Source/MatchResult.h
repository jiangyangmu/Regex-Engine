#pragma once

#include "RegexSyntax.h"

class CaptureGroup {
public:
    explicit CaptureGroup(RView origin)
        : origin_(origin)
        , need_begin_(true) {
    }

    // access functions
    RView GetLast() const {
        auto range = GetLastRange();
        return origin_.subview(range.first, range.second - range.first);
    }
    std::pair<size_t, size_t> GetLastRange() const {
        assert(IsReady() && !captured_.empty());
        return captured_.back();
    }

    // build functions
    void Add(size_t pos, bool is_begin) {
        if (is_begin && need_begin_)
        {
            captured_.push_back({pos, 0});
            need_begin_ = false;
        }
        else if (!is_begin && !need_begin_)
        {
            captured_.back().second = pos;
            need_begin_ = true;
        }
        events_.emplace_back(is_begin, pos);
    }

    const std::vector<std::pair<size_t, size_t>> & captured() const {
        return captured_;
    }
    const std::vector<std::pair<bool, size_t>> & events() const {
        return events_;
    }

private:
    bool IsReady() const {
        return need_begin_;
    }

    RView origin_;
    bool need_begin_;
    std::vector<std::pair<size_t, size_t>> captured_;
    std::vector<std::pair<bool, size_t>> events_;
};

class Capture {
public:
    explicit Capture(RView origin)
        : origin_(origin) {
    }

    RView Origin() const {
        return origin_;
    }
    const CaptureGroup & Group(size_t group_id) const {
        assert(group_id < capture_groups_.size());
        return capture_groups_.at(group_id);
    }
    void CopyTo(Capture & c) const {
        c.~Capture();
        ::new (&c) Capture(origin_);
        c.capture_groups_ = capture_groups_;
    }

    void DoCapture(size_t group_id, size_t pos, bool is_begin) {
        if (capture_groups_.find(group_id) == capture_groups_.end())
            capture_groups_.emplace(group_id, CaptureGroup(origin_));
        capture_groups_.at(group_id).Add(pos, is_begin);
    }

    RString DebugString() const {
        RString s;
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
#ifdef DEBUG
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
#endif
        return s;
    }

private:
    RView origin_;
    std::map<size_t, CaptureGroup> capture_groups_;
};

class MatchResult {
public:
    MatchResult(const ::Capture & capture, bool matched)
        : capture_(capture)
        , matched_(matched) {
    }

    bool Matched() const {
        return matched_;
    }
    const ::Capture & GetCapture() const {
        return capture_;
    }

private:
    bool matched_;
    ::Capture capture_;
};
