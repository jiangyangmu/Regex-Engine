#include "stdafx.h"

#include <functional>

#include "compile.h"
#include "enfa.h"

namespace v1 {

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

        // adjust pos based on look around

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

} // namespace v1

namespace v2 {

std::string EnfaState::DebugString(EnfaState * start) {
    std::map<const EnfaState *, int> m;
    std::vector<EnfaState *> v = {start};
    int id = 0;
    while (!v.empty())
    {
        EnfaState * s = v.back();
        v.pop_back();

        if (s && m.find(s) == m.end())
        {
            m[s] = id++;
            v.insert(v.end(), s->forward.out_.rbegin(), s->forward.out_.rend());
        }
    }

    std::map<int, std::string> out;
    for (auto kv : m)
    {
        std::string & o = out[kv.second];
        if (kv.first->IsChar())
            o += "'", o += kv.first->Char(),
                o += "' [" + std::to_string(m[kv.first->Out()]) + "]";
        else if (kv.first->IsBackReference())
            o += "\\", o += std::to_string(kv.first->BackReference()),
                o += " [" + std::to_string(m[kv.first->Out()]) + "]";
        else if (kv.first->IsEpsilon())
        {
            o += "<   ";
            for (EnfaState * out : kv.first->MultipleOut())
                o += "[" + std::to_string(m[out]) + "]";
        }

        if (kv.first->IsFinal())
            o += " FINAL";
        if (kv.first->HasCaptureTag())
            o += std::string("\t") +
                (kv.first->GetCaptureTag().is_begin ? "B" : "E") +
                std::to_string(kv.first->GetCaptureTag().capture_id);
        if (kv.first->HasLookAroundTag())
            o += std::string("\t") +
                (kv.first->GetLookAroundTag().is_begin ? "LB" : "LE") +
                std::to_string(kv.first->GetLookAroundTag().id);
    }

    std::string s;
    for (auto kv : out)
        s += "[" + std::to_string(kv.first) + "] " + kv.second + "\n";
    return s;
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Char(char c) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();
    in->forward = out->backword = {
        EnfaState::CHAR_OUT,
        c,
        0,
    };
    in->forward.out_.push_back(out);
    out->backword.out_.push_back(in);
    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::BackReference(
    size_t ref_capture_id) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();
    in->forward = out->backword = {
        EnfaState::BACKREF_OUT,
        0,
        ref_capture_id,
    };
    in->forward.out_.push_back(out);
    out->backword.out_.push_back(in);
    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Alter(StatePort sp1,
                                                    StatePort sp2) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->forward = sp1.in->backword = sp2.in->backword = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    in->forward.out_.push_back(sp1.in);
    in->forward.out_.push_back(sp2.in);
    sp1.in->backword.out_.push_back(in);
    sp2.in->backword.out_.push_back(in);

    out->backword = sp1.out->forward = sp2.out->forward = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    out->backword.out_.push_back(sp1.out);
    out->backword.out_.push_back(sp2.out);
    sp1.out->forward.out_.push_back(out);
    sp2.out->forward.out_.push_back(out);

    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Repeat(StatePort sp) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->forward = out->backword = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp.in->backword = sp.out->forward = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    in->forward.out_.push_back(sp.in);
    in->forward.out_.push_back(out);

    out->backword.out_.push_back(sp.out);
    out->backword.out_.push_back(in);

    sp.in->backword.out_.push_back(sp.out);
    sp.in->backword.out_.push_back(in);

    sp.out->forward.out_.push_back(sp.in);
    sp.out->forward.out_.push_back(out);

    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Concat(StatePort sp1,
                                                     StatePort sp2) {
    sp1.out->forward = sp2.in->backword = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    sp1.out->forward.out_.push_back(sp2.in);
    sp2.in->backword.out_.push_back(sp1.out);

    return {sp1.in, sp2.out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Group(StatePort sp) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->forward = out->backword = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp.in->backword = sp.out->forward = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    in->forward.out_.push_back(sp.in);
    sp.in->backword.out_.push_back(in);

    out->backword.out_.push_back(sp.out);
    sp.out->forward.out_.push_back(out);

    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::InverseGroup(StatePort sp) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->forward = out->backword = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp.in->backword = sp.out->forward = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    in->forward.out_.push_back(sp.out);
    sp.out->forward.out_.push_back(in);

    out->backword.out_.push_back(sp.in);
    sp.in->backword.out_.push_back(out);

    return {in, out};
}

#define DEBUG
#ifdef DEBUG
std::map<const EnfaState *, int> BuildIdMap(const EnfaState * start) {
    std::map<const EnfaState *, int> m;
    std::vector<const EnfaState *> v = {start};
    int id = 0;
    while (!v.empty())
    {
        const EnfaState * s = v.back();
        v.pop_back();

        if (s && m.find(s) == m.end())
        {
            m[s] = id++;
            v.insert(v.end(),
                     s->DebugMultipleOut().rbegin(),
                     s->DebugMultipleOut().rend());
        }
    }
    return m;
}
#endif

struct Thread {
    size_t pos;
    const EnfaState * state;
    Capture capture;
};

MatchResult MatchWhen(const Thread & start,
                      std::function<bool(const EnfaState & state)> pred,
                      bool forward_match) {
    const std::string & text = start.capture.origin();

#ifdef DEBUG
    static std::map<const EnfaState *, int> debug;
    if (debug.empty())
        debug = BuildIdMap(start.state);
#endif

    std::vector<Thread> T = {start};
    while (!T.empty())
    {
        Thread t = T.back();
        T.pop_back();

        (forward_match ? t.state->SetForwardMode()
                       : t.state->SetBackwordMode());

        if (t.state->HasCaptureTag())
            t.capture.DoCapture(t.state->GetCaptureTag(), t.pos);

        if (pred(*t.state))
            return MatchResult(t.capture, true);

        // look around assertion
        if (t.state->HasLookAroundTag() && t.state->GetLookAroundTag().is_begin)
        {
#ifdef DEBUG
            std::cout << "Eval "
                      << (t.state->GetLookAroundTag().is_forward
                              ? "lookahead "
                              : "lookbehind ")
                      << t.state->GetLookAroundTag().id << " at " << t.pos
                      << std::endl;
#endif

            int lookaround_id = t.state->GetLookAroundTag().id;
            bool is_forward = t.state->GetLookAroundTag().is_forward;
            const EnfaState * last_state = nullptr;
            t.state = t.state->Out();
            if (MatchWhen(t,
                          [lookaround_id, &last_state](
                              const EnfaState & state) mutable -> bool {
                              last_state = &state;
                              return state.HasLookAroundTag() &&
                                  !state.GetLookAroundTag().is_begin &&
                                  state.GetLookAroundTag().id == lookaround_id;
                          },
                          is_forward)
                    .matched())
            {
                assert(last_state);
                t.state = last_state;

                (forward_match ? t.state->SetForwardMode()
                               : t.state->SetBackwordMode());
            }
            else
            { continue; }
        }

#ifdef DEBUG
        std::cout << debug[t.state] << ' ';
#endif

        if (t.state->IsChar())
        {
            if (forward_match)
            {
                if (t.pos < text.size() && t.state->Char() == text[t.pos])
                    T.push_back({t.pos + 1, t.state->Out(), t.capture})
#ifdef DEBUG
                        ,
                        std::cout << '+' << debug[t.state->Out()]
#endif
                        ;
            }
            else
            {
                if (t.pos > 0 && t.state->Char() == text[t.pos - 1])
                    T.push_back({t.pos - 1, t.state->Out(), t.capture})
#ifdef DEBUG
                        ,
                        std::cout << '+' << debug[t.state->Out()]
#endif
                        ;
            }
        }
        else if (t.state->IsBackReference())
        {
            auto group = t.capture.Group(t.state->BackReference());
            if (group.IsComplete())
            {
                auto range = group.Last();
                assert(range.second >= range.first);
                size_t delta = range.second - range.first;
                if (forward_match)
                {
                    bool match = (t.pos == range.first) ||
                        (t.pos + delta <= text.size() &&
                         std::equal(text.data() + range.first,
                                    text.data() + range.second,
                                    text.data() + t.pos,
                                    text.data() + t.pos + delta));
                    if (match)
                        T.push_back({t.pos + delta, t.state->Out(), t.capture});
                }
                else
                {
                    bool match = (t.pos == range.second) ||
                        (t.pos >= delta &&
                         std::equal(text.data() + range.first,
                                    text.data() + range.second,
                                    text.data() + t.pos - delta,
                                    text.data() + t.pos));
                    if (match)
                        T.push_back({t.pos - delta, t.state->Out(), t.capture});
                }
            }
        }
        else if (t.state->IsEpsilon())
        {
            for (auto out = t.state->MultipleOut().rbegin();
                 out != t.state->MultipleOut().rend();
                 ++out)
            {
                T.push_back({t.pos, *out, t.capture})
#ifdef DEBUG
                    ,
                    std::cout << '+' << debug[*out]
#endif
                    ;
            }
        }
#ifdef DEBUG
        std::cout << std::endl;
#endif
    }
    return MatchResult(Capture(start.capture.origin()), false);
}

MatchResult ENFA::Match(const std::string & text) const {
    Thread t = {0, start_, Capture(text)};
    return MatchWhen(
        t,
        [](const EnfaState & state) -> bool { return state.IsFinal(); },
        true);
}

} // namespace v2
