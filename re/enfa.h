#pragma once

#include "stdafx.h"

// epsilon-NFA state
struct State {
    enum Type { NONE, SPLIT = 256, FINAL = 257 };
    // char, out, out1, final
    int c;
    State * out;
    State * out1;

    // capturing group tag
    std::deque<int> * btags;
    std::deque<int> * etags;

    static State * NewCharState(char c) {
        State * s = new State;
        s->c = c;
        s->out = s->out1 = nullptr;
        s->btags = s->etags = nullptr;
        return s;
    }
    static State * NewSplitState(State * out, State * out1) {
        State * s = new State;
        s->c = SPLIT;
        s->out = out;
        s->out1 = out1;
        s->btags = s->etags = nullptr;
        return s;
    }
    static State * NewFinalState() {
        State * s = new State;
        s->c = FINAL;
        s->out = s->out1 = nullptr;
        s->btags = s->etags = nullptr;
        return s;
    }

    void AddBeginTag(int group_id) {
        if (!btags)
            btags = new std::deque<int>();
        btags->push_front(group_id);
    }
    void AddEndTags(const std::deque<int> & group_ids) {
        if (!etags)
            etags = new std::deque<int>(group_ids);
        else
            etags->insert(etags->end(), group_ids.begin(), group_ids.end());
    }

    static std::string DebugString(State * start) {
        std::map<State *, int> m;
        std::vector<State *> v = {start};
        int id = 0;
        while (!v.empty())
        {
            State * s = v.back();
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
                o += "<   [" + std::to_string(m[kv.first->out]) + "] [" +
                    std::to_string(m[kv.first->out1]) + "]";
            else if (kv.first->c == FINAL)
                o += "o";
            if (kv.first->btags)
            {
                o += " btags:";
                for (int tag : *kv.first->btags)
                    o += std::to_string(tag) + ",";
            }
            if (kv.first->etags)
            {
                o += " etags:";
                for (int tag : *kv.first->etags)
                    o += std::to_string(tag) + ",";
            }
        }

        std::string s;
        for (auto kv : out)
            s += "[" + std::to_string(kv.first) + "] " + kv.second + "\n";
        return s;
    }
};
