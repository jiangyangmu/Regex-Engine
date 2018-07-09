#include "stdafx.h"

#include "compile.h"
#include "enfa.h"

std::string EnfaState::DebugString(EnfaState * start) {
    std::map<EnfaState *, int> m;
    std::vector<EnfaState *> v = {start};
    int id = 0;
    while (!v.empty())
    {
        EnfaState * s = v.back();
        v.pop_back();

        if (s && m.find(s) == m.end())
        {
            m[s] = id++;
            v.push_back(s->out1_);
            v.push_back(s->out_);
        }
    }

    std::map<int, std::string> out;
    for (auto kv : m)
    {
        std::string & o = out[kv.second];
        if (kv.first->IsChar())
            o += "'", o += kv.first->type_,
                o += "' [" + std::to_string(m[kv.first->out_]) + "]";
        else if (kv.first->IsBackReference())
            o += "\\", o += (kv.first->type_ - BACKREF_0_ + '0'),
                o += " [" + std::to_string(m[kv.first->out_]) + "]";
        else if (kv.first->IsSplit())
            o += "<   [" + std::to_string(m[kv.first->out_]) + "][" +
                std::to_string(m[kv.first->out1_]) + "]";
        else if (kv.first->IsFinal())
            o += "o";
        o += "\t[" + kv.first->tag_.DebugString() + "]";
    }

    std::string s;
    for (auto kv : out)
        s += "[" + std::to_string(kv.first) + "] " + kv.second + "\n";
    return s;
}

EnfaState * EnfaStateBuilder::NewCharState(char c) {
    EnfaState * s = new EnfaState;
    s->type_ = c;
    s->out_ = s->out1_ = nullptr;
    return s;
}

EnfaState * EnfaStateBuilder::NewBackReferenceState(size_t ref_group_id) {
    assert(ref_group_id >= 0 && ref_group_id <= 9);
    EnfaState * s = new EnfaState;
    s->type_ = EnfaState::BACKREF_0_ + ref_group_id;
    s->out_ = s->out1_ = nullptr;
    return s;
}

EnfaState * EnfaStateBuilder::NewSplitState(EnfaState * out, EnfaState * out1) {
    EnfaState * s = new EnfaState;
    s->type_ = EnfaState::SPLIT;
    s->out_ = out;
    s->out1_ = out1;
    return s;
}

EnfaState * EnfaStateBuilder::NewFinalState() {
    EnfaState * s = new EnfaState;
    s->type_ = EnfaState::FINAL;
    s->out_ = s->out1_ = nullptr;
    return s;
}

MatchResult ENFA::Match(const std::string & text) const {
    struct Thread {
        size_t pos;
        const EnfaState * state;
        Capture capture;
    };

    std::deque<Thread> T = {{0, start_, Capture(text)}};
    while (!T.empty())
    {
        Thread t = T.front();
        T.pop_front();

        t.capture.DoCapture(t.state->Tag(), t.pos);

        if (t.state->IsChar())
        {
            if (t.pos < text.size() && t.state->Char() == text[t.pos])
                T.push_back({t.pos + 1, t.state->Out(), t.capture});
        }
        else if (t.state->IsBackReference())
        {
            auto group = t.capture.Group(t.state->BackReference());
            if (group.IsComplete())
            {
                auto range = group.Last();
                int delta = range.second - range.first;
                bool match = (t.pos == range.first) ||
                    (t.pos + delta <= text.size() &&
                     std::equal(text.data() + range.first,
                                text.data() + range.second,
                                text.data() + t.pos,
                                text.data() + t.pos + delta));
                if (match)
                    T.push_back({t.pos + delta, t.state->Out(), t.capture});
            }
        }
        else if (t.state->IsSplit())
        {
            T.push_back({t.pos, t.state->Out(), t.capture});
            T.push_back({t.pos, t.state->Out1(), t.capture});
        }
        else if (t.state->IsFinal())
        {
            if (t.pos == text.size())
                return MatchResult(t.capture, true);
        }
    }
    return MatchResult(Capture(text), false);
}
