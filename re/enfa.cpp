#include "stdafx.h"

#include "enfa.h"
#include "compile.h"

std::string EnfaState::DebugString(EnfaState * start)
{
    std::map<EnfaState *, int> m;
    std::vector<EnfaState *> v = { start };
    int id = 0;
    while (!v.empty())
    {
        EnfaState * s = v.back();
        v.pop_back();

        if (s && m.find(s) == m.end())
        {
            m[s] = id++;
            v.push_back(s->out1);
            v.push_back(s->out);
        }
    }

    std::map<int, std::string> out;
    for (auto kv : m)
    {
        std::string & o = out[kv.second];
        if (kv.first->c < 256)
            o += "'", o += kv.first->c,
            o += "' [" + std::to_string(m[kv.first->out]) + "]";
        else if (kv.first->c == SPLIT)
            o += "<   [" + std::to_string(m[kv.first->out]) + "][" +
            std::to_string(m[kv.first->out1]) + "]";
        else if (kv.first->c == FINAL)
            o += "o";
        o += "\t[" + kv.first->tag_.DebugString() + "]";
    }

    std::string s;
    for (auto kv : out)
        s += "[" + std::to_string(kv.first) + "] " + kv.second + "\n";
    return s;
}

EnfaState * EnfaStateBuilder::NewCharState(char c)
{
    EnfaState * s = new EnfaState;
    s->c = c;
    s->out = s->out1 = nullptr;
    return s;
}

EnfaState * EnfaStateBuilder::NewSplitState(EnfaState * out, EnfaState * out1)
{
    EnfaState * s = new EnfaState;
    s->c = EnfaState::SPLIT;
    s->out = out;
    s->out1 = out1;
    return s;
}

EnfaState * EnfaStateBuilder::NewFinalState()
{
    EnfaState * s = new EnfaState;
    s->c = EnfaState::FINAL;
    s->out = s->out1 = nullptr;
    return s;
}

MatchResult ENFA::Match(const std::string & text) const
{
    struct Thread {
        size_t pos;
        const EnfaState * state;
        Capture capture;
    };

    std::deque<Thread> T = { { 0, start_, Capture(text) } };
    while (!T.empty())
    {
        Thread t = T.front();
        T.pop_front();

        t.capture.DoCapture(t.state->Tag(), t.pos);

        int sc = t.state->c;
        if (t.state->IsChar())
        {
            if (t.pos < text.size() && t.state->Char() == text[t.pos])
                T.push_back({ t.pos + 1, t.state->Out(), t.capture });
        }
        else if (t.state->IsSplit())
        {
            T.push_back({ t.pos, t.state->Out(), t.capture });
            T.push_back({ t.pos, t.state->Out1(), t.capture });
        }
        else if (t.state->IsFinal())
        {
            if (t.pos == text.size())
                return MatchResult(t.capture, true);
        }
    }
    return MatchResult(Capture(text), false);
}
