#pragma once

class Capture;

namespace v1 {

class CaptureTag {
    friend class Capture;

public:
    CaptureTag()
        : begin_(0ull)
        , end_(0ull) {
    }

    void AddBegin(size_t group_id) {
        assert(group_id < 64);
        begin_ |= (1ull << group_id);
    }
    void AddEnd(size_t group_id) {
        assert(group_id < 64);
        end_ |= (1ull << group_id);
    }
    void Merge(const CaptureTag & tag) {
        begin_ |= tag.begin_;
        end_ |= tag.end_;
    }

    std::string DebugString() const {
        std::string s;
        size_t group_id = 0;
        uint64_t mask = begin_;
        s += "begin: ";
        while (mask > 0)
        {
            if (mask & 0x1)
                s += std::to_string(group_id) + " ";
            mask >>= 1;
            ++group_id;
        }

        group_id = 0;
        mask = end_;
        s += "end: ";
        while (mask > 0)
        {
            if (mask & 0x1)
                s += std::to_string(group_id) + " ";
            mask >>= 1;
            ++group_id;
        }
        if (!s.empty() && s.back() == ' ')
            s.pop_back();
        return s;
    }

private:
    uint64_t begin_;
    uint64_t end_;
};

} // namespace v1

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
    Capture(const std::string & origin)
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

    void DoCapture(const v1::CaptureTag & tag, size_t pos) {
        size_t group_id = 0;
        uint64_t mask = tag.begin_;
        while (mask > 0)
        {
            if (mask & 0x1)
                capture_groups_[group_id].Begin(pos);
            mask >>= 1;
            ++group_id;
        }

        group_id = 0;
        mask = tag.end_;
        while (mask > 0)
        {
            if (mask & 0x1)
                capture_groups_[group_id].End(pos);
            mask >>= 1;
            ++group_id;
        }
    }
    void DoCapture(const v2::CaptureTag & tag, size_t pos) {
        if (tag.is_begin)
            capture_groups_[tag.capture_id].Begin(pos);
        else
            capture_groups_[tag.capture_id].End(pos);
    }

    const std::string & origin() const {
        return origin_;
    }

    std::string DebugString() const {
        std::string s;
        for (const auto & kv : capture_groups_)
        {
            s += std::to_string(kv.first) + ":";
            for (const auto & range : kv.second.captured())
            {
                s += " \"";
                s += origin_.substr(range.first, range.second - range.first);
                s += "\"";
            }
            s += "\n";
        }
        for (const auto & kv : capture_groups_)
        {
            s += std::to_string(kv.first) + ":";
            for (const auto & p : kv.second.events())
            {
                s += ' ';
                s += (p.first ? 'B' : 'E');
                s += ':';
                s += std::to_string(p.second);
            }
            s += "\n";
        }
        return s;
    }

private:
    const std::string & origin_;
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
