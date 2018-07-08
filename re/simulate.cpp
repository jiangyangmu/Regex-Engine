#include "stdafx.h"

#include "enfa.h"

std::deque<State *> split(std::deque<State *> before) {
    std::deque<State *> after;
    while (!before.empty())
    {
        State * s = before.front();
        before.pop_front();
        if (s->c == State::SPLIT)
        {
            before.push_back(s->out);
            before.push_back(s->out1);
        }
        else
        {
            after.push_back(s);
        }
    }
    return after;
}

bool is_final(const std::deque<State *> & states) {
    for (State * s : states)
    {
        if (s->c == State::FINAL)
            return true;
    }
    return false;
}

bool Match(State * enfa, const std::string & input) {
    std::deque<State *> S = { enfa };
    for (char c : input)
    {
        std::deque<State *> S1 = split(S);
        std::set<State *> S2;
        for (State * s : S1)
        {
            if (s->c == c)
                S2.insert(s->out);
        }
        S.clear();
        S.insert(S.begin(), S2.begin(), S2.end());
    }
    return is_final(split(S));
}

bool MatchWithCapture(State * enfa, const std::string & input) {
    struct Thread {
        size_t pos;
        State * state;
        Capture capture;
    };
    std::deque<Thread> T = { {0, enfa, Capture(input)} };
    while (!T.empty())
    {
        if (T.size() > 100) {
            std::cout << "too many states." << std::endl;
            return false;
        }
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
            if (t.pos < input.size() && sc == input[t.pos])
                T.push_back({ t.pos + 1, t.state->out, t.capture });
        }
        else if (sc == State::SPLIT)
        {
            T.push_back({ t.pos, t.state->out, t.capture });
            T.push_back({ t.pos, t.state->out1, t.capture });
        }
        else if (sc == State::FINAL)
        {
            if (t.pos == input.size())
            {
                std::cout << t.capture.DebugString() << std::endl;
                return true;
            }
        }
    }
    return false;
}
