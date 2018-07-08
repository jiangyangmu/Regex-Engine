#include "stdafx.h"

#include "enfa.h"

class Capture {
public:
    class Group {
    public:
        Group()
            : need_begin_(true) {
        }
        void Begin(size_t pos) {
            if (need_begin_)
            {
                captured_.push_back({pos, 0});
                need_begin_ = false;
            }
        }
        void End(size_t pos) {
            if (!need_begin_)
            {
                captured_.back().second = pos;
                need_begin_ = true;
            }
        }
        bool Empty() const {
            return captured_.empty();
        }
        const std::vector<std::pair<size_t, size_t>> & captured() const {
            return captured_;
        }

    private:
        bool need_begin_;
        std::vector<std::pair<size_t, size_t>> captured_;
    };

public:
    Capture(const std::string & origin)
        : origin_(origin) {
    }

    void Begin(int group_id, size_t pos) {
        capture_events_[group_id].push_back({true, pos});
        capture_groups_[group_id].Begin(pos);
    }
    void End(int group_id, size_t pos) {
        capture_events_[group_id].push_back({false, pos});
        capture_groups_[group_id].End(pos);
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
            for (auto p : capture_events_.at(kv.first))
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
    std::map<size_t, Group> capture_groups_;

    std::map<size_t, std::vector<std::pair<bool, size_t>>> capture_events_;
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
    Capture capture_;
    bool matched_;
};

class ENFA {
    friend class FABuilder;

public:
    MatchResult Match(const std::string & text) const {
        struct Thread {
            size_t pos;
            State * state;
            Capture capture;
        };

        std::deque<Thread> T = {{0, start_, Capture(text)}};
        while (!T.empty())
        {
            Thread t = T.front();
            T.pop_front();

            auto btags = t.state->btags;
            auto etags = t.state->etags;
            if (btags)
            {
                for (int tag : *btags)
                    t.capture.Begin(tag, t.pos);
            }
            if (etags)
            {
                for (int tag : *etags)
                    t.capture.End(tag, t.pos);
            }

            int sc = t.state->c;
            if (sc < 256)
            {
                if (t.pos < text.size() && sc == text[t.pos])
                    T.push_back({t.pos + 1, t.state->out, t.capture});
            }
            else if (sc == State::SPLIT)
            {
                T.push_back({t.pos, t.state->out, t.capture});
                T.push_back({t.pos, t.state->out1, t.capture});
            }
            else if (sc == State::FINAL)
            {
                if (t.pos == text.size())
                {
                    std::cout << t.capture.DebugString() << std::endl;
                    return MatchResult(t.capture, true);
                }
            }
        }
        return MatchResult(Capture(text), false);
    }

private:
    State * start_;
};

extern State * RegexToEnfa(const std::string & input);
extern void ConvertDebugPrint(std::string & regex);

class FABuilder {
public:
    static ENFA Compile(std::string regex) {
        ENFA enfa;
        enfa.start_ = RegexToEnfa(regex);
        return enfa;
    }
};

int main() {
    // std::string regex = "(ab*|c)";
    // std::string regex = "((a)*)";
    // std::string regex = "((a(b*)c)*)";
    // std::string regex = "((a*)|(b*)|(c*))";
    // std::string regex = "((a*)|(a)*|(a*)*)";
    std::string regex = "((a(b)*c|d(e*)f)*)";
    // std::string regex = "((aa*)|(a*))";
    ConvertDebugPrint(regex);

    ENFA enfa = FABuilder::Compile(regex);

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        MatchResult m1 = enfa.Match(line);
        if (m1.matched())
            std::cout << m1.capture().DebugString() << std::endl;
        std::cout << "ENFA: " << (m1.matched() ? "ok." : "reject.")
                  << std::endl;
    }

    return 0;
}
