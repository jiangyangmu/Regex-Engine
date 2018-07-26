#include "stdafx.h"

#include <functional>

#include "compile.h"
#include "enfa.h"

namespace v2 {

CharArray EnfaState::DebugString(EnfaState * start) {
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

    std::map<int, CharArray> out;
    for (auto kv : m)
    {
        CharArray & o = out[kv.second];
        if (kv.first->IsChar())
            o += L"'", o += kv.first->Char(),
                o += L"' [" + std::to_wstring(m[kv.first->Out()]) + L"]";
        else if (kv.first->IsBackReference())
            o += L"\\", o += std::to_wstring(kv.first->BackReference()),
                o += L" [" + std::to_wstring(m[kv.first->Out()]) + L"]";
        else if (kv.first->IsEpsilon())
        {
            o += L"<   ";
            for (EnfaState * out : kv.first->MultipleOut())
                o += L"[" + std::to_wstring(m[out]) + L"]";
        }

        if (kv.first->IsFinal())
            o += L" FINAL";
        if (kv.first->HasCaptureTag())
            o += std::wstring(L"\t") +
                (kv.first->GetCaptureTag().is_begin ? L"B" : L"E") +
                std::to_wstring(kv.first->GetCaptureTag().capture_id);
        if (kv.first->HasLookAroundTag())
            o += std::wstring(L"\t") +
                (kv.first->GetLookAroundTag().is_begin ? L"LB" : L"LE") +
                std::to_wstring(kv.first->GetLookAroundTag().id);
        if (kv.first->HasAtomicTag())
            o += std::wstring(L"\t") +
                (kv.first->GetAtomicTag().is_begin ? L"AB" : L"AE") +
                std::to_wstring(kv.first->GetAtomicTag().atomic_id);
        if (kv.first->HasRepeatTag())
            o += std::wstring(L"\t") + L"{" +
                std::to_wstring(kv.first->GetRepeatTag().min) + L"," +
                (kv.first->GetRepeatTag().has_max
                     ? std::to_wstring(kv.first->GetRepeatTag().max)
                     : L"") +
                L"}";
    }

    CharArray s;
    for (auto kv : out)
        s += L"[" + std::to_wstring(kv.first) + L"] " + kv.second + L"\n";
    return s;
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Char(::Char::Type c) {
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

EnfaStateBuilder::StatePort EnfaStateBuilder::Repeat(StatePort sp,
                                                     struct Repeat rep) {
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
    if (rep.min == 0)
        in->forward.out_.push_back(out);

    out->backword.out_.push_back(sp.out);
    if (rep.min == 0)
        out->backword.out_.push_back(in);

    sp.in->backword.out_.push_back(sp.out);
    sp.in->backword.out_.push_back(in);

    // Match algorithm depends on push order here.
    sp.out->forward.out_.push_back(sp.in);
    sp.out->forward.out_.push_back(out);

    sp.out->SetRepeatTag({rep.repeat_id, rep.min, rep.max, rep.has_max});

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
    std::vector<std::pair<int, size_t>> id_to_repcnt;
};

MatchResult MatchWhen(const Thread & start,
                      std::function<bool(const Thread &)> pred,
                      bool forward_match) {
    const CharArray & text = start.capture.origin();

#ifdef DEBUG_STATS
    static size_t generated_threads = 0;
    static size_t processed_threads = 0;
    static std::vector<size_t> stat_save;
#endif
#ifdef DEBUG
    static std::map<const EnfaState *, int> debug;
    static std::wstring debug_indent;
    if (debug.empty())
        debug = BuildIdMap(start.state);
#endif

    std::vector<Thread> T = {start};
    while (!T.empty())
    {
#ifdef DEBUG_STATS
        ++processed_threads;
#endif
        Thread t = T.back();
        T.pop_back();
#ifdef DEBUG
        std::wcout << debug_indent;
        std::wcout << text << std::endl;
        std::wcout << debug_indent << std::wstring(t.pos, ' ') << '^'
                   << std::endl;
#endif

        (forward_match ? t.state->SetForwardMode()
                       : t.state->SetBackwordMode());

        if (t.state->HasCaptureTag())
            t.capture.DoCapture(t.state->GetCaptureTag(), t.pos);

        if (pred(t))
        {
#ifdef DEBUG_STATS
            generated_threads = processed_threads + T.size();
            std::wcout << "Generated Threads: " << generated_threads
                       << "\tProcessed Threads: " << processed_threads
                       << std::endl;
            generated_threads = processed_threads = 0;
#endif
#ifdef DEBUG
            if (debug_indent.size() >= 2)
                debug_indent.pop_back(), debug_indent.pop_back();
#endif
            return MatchResult(t.capture, true);
        }

        // look around assertion
        if (t.state->HasLookAroundTag() && t.state->GetLookAroundTag().is_begin)
        {
#ifdef DEBUG
            std::wcout << L"Eval "
                       << (t.state->GetLookAroundTag().is_forward
                               ? L"lookahead "
                               : L"lookbehind ")
                       << t.state->GetLookAroundTag().id << L" at " << t.pos
                       << std::endl;
            debug_indent += L"  ";
#endif

            int lookaround_id = t.state->GetLookAroundTag().id;
            bool is_forward = t.state->GetLookAroundTag().is_forward;
            Thread start = {t.pos, t.state->Out(), t.capture};
#ifdef DEBUG_STATS
            stat_save.push_back(generated_threads);
            stat_save.push_back(processed_threads);
            generated_threads = processed_threads = 0;
#endif
            if (MatchWhen(start,
                          [lookaround_id,
                           &t](const Thread & current) mutable -> bool {
                              bool good = current.state->HasLookAroundTag() &&
                                  !current.state->GetLookAroundTag().is_begin &&
                                  current.state->GetLookAroundTag().id ==
                                      lookaround_id;
                              if (good)
                                  t.state = current.state;
                              return good;
                          },
                          is_forward)
                    .matched())
            {
#ifdef DEBUG_STATS
                processed_threads = stat_save.back();
                stat_save.pop_back();
                generated_threads = stat_save.back();
                stat_save.pop_back();
#endif
                (forward_match ? t.state->SetForwardMode()
                               : t.state->SetBackwordMode());
            }
            else
            {
#ifdef DEBUG_STATS
                processed_threads = stat_save.back();
                stat_save.pop_back();
                generated_threads = stat_save.back();
                stat_save.pop_back();
#endif
                continue;
            }
        }
        else if (t.state->HasAtomicTag() && t.state->GetAtomicTag().is_begin)
        {
#ifdef DEBUG
            std::wcout << "Eval atomic " << t.state->GetAtomicTag().atomic_id
                       << " at " << t.pos << std::endl;
            debug_indent += L"  ";
#endif

            int atomic_id = t.state->GetAtomicTag().atomic_id;
            Thread start = {t.pos, t.state->Out(), t.capture};
#ifdef DEBUG_STATS
            stat_save.push_back(generated_threads);
            stat_save.push_back(processed_threads);
            generated_threads = processed_threads = 0;
#endif
            if (MatchWhen(
                    start,
                    [atomic_id, &t](const Thread & current) mutable -> bool {
                        bool good = current.state->HasAtomicTag() &&
                            !current.state->GetAtomicTag().is_begin &&
                            current.state->GetAtomicTag().atomic_id ==
                                atomic_id;
                        if (good)
                        {
                            t.pos = current.pos;
                            t.state = current.state;
                            current.capture.CopyTo(t.capture);
                        }
                        return good;
                    },
                    true)
                    .matched())
            {
#ifdef DEBUG_STATS
                processed_threads = stat_save.back();
                stat_save.pop_back();
                generated_threads = stat_save.back();
                stat_save.pop_back();
#endif
            }
            else
            {
#ifdef DEBUG_STATS
                processed_threads = stat_save.back();
                stat_save.pop_back();
                generated_threads = stat_save.back();
                stat_save.pop_back();
#endif
                continue;
            }
        }

#ifdef DEBUG
        std::wcout << debug_indent << debug[t.state] << ' ';
#endif

        if (t.state->IsChar())
        {
            if (forward_match)
            {
                if (t.pos < text.size() && t.state->Char() == text[t.pos])
                    T.push_back(
                        {t.pos + 1, t.state->Out(), t.capture, t.id_to_repcnt})
#ifdef DEBUG
                        ,
                        std::wcout << '+' << debug[t.state->Out()]
#endif
                        ;
            }
            else
            {
                if (t.pos > 0 && t.state->Char() == text[t.pos - 1])
                    T.push_back(
                        {t.pos - 1, t.state->Out(), t.capture, t.id_to_repcnt})
#ifdef DEBUG
                        ,
                        std::wcout << '+' << debug[t.state->Out()]
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
                        T.push_back({t.pos + delta,
                                     t.state->Out(),
                                     t.capture,
                                     t.id_to_repcnt});
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
                        T.push_back({t.pos - delta,
                                     t.state->Out(),
                                     t.capture,
                                     t.id_to_repcnt});
                }
            }
        }
        else if (t.state->IsEpsilon())
        {
            if (t.state->HasRepeatTag())
            {
                auto repeat = t.state->GetRepeatTag();
                if (t.id_to_repcnt.empty() ||
                    t.id_to_repcnt.back().first != repeat.repeat_id)
                {
                    t.id_to_repcnt.emplace_back(repeat.repeat_id, 0);
                }
                t.id_to_repcnt.back().second += 1;

                auto outs = t.state->MultipleOut();
                assert(outs.size() == 2);

                size_t current = t.id_to_repcnt.back().second;
#ifdef DEBUG
                std::wcout << " repeat:" << current << "{" << repeat.min << ","
                           << (repeat.has_max ? std::to_wstring(repeat.max)
                                              : L"")
                           << "} ";
#endif
                if (current < repeat.min)
                    T.push_back({t.pos, outs[0], t.capture, t.id_to_repcnt})
#ifdef DEBUG
                        ,
                        std::wcout << '+' << debug[outs[0]]
#endif
                        ;
                else if (!repeat.has_max || current < repeat.max)
                {
                    T.push_back({t.pos, outs[1], t.capture, t.id_to_repcnt});
                    T.back().id_to_repcnt.pop_back();
                    T.push_back({t.pos, outs[0], t.capture, t.id_to_repcnt});
#ifdef DEBUG
                    std::wcout << '+' << debug[outs[1]] << '+'
                               << debug[outs[0]];
#endif
                }
                else
                {
                    T.push_back({t.pos, outs[1], t.capture, t.id_to_repcnt});
                    T.back().id_to_repcnt.pop_back();
#ifdef DEBUG
                    std::wcout << '+' << debug[outs[1]];
#endif
                }
            }
            else
            {
                for (auto out = t.state->MultipleOut().rbegin();
                     out != t.state->MultipleOut().rend();
                     ++out)
                {
                    T.push_back({t.pos, *out, t.capture, t.id_to_repcnt})
#ifdef DEBUG
                        ,
                        std::wcout << '+' << debug[*out]
#endif
                        ;
                }
            }
        }
#ifdef DEBUG
        std::wcout << std::endl;
#endif
    }

#ifdef DEBUG_STATS
    generated_threads = processed_threads + T.size();
    std::wcout << "Generated Threads: " << generated_threads
               << "\tProcessed Threads: " << processed_threads << std::endl;
    generated_threads = processed_threads = 0;
#endif
#ifdef DEBUG
    if (debug_indent.size() >= 2)
        debug_indent.pop_back(), debug_indent.pop_back();
#endif
    return MatchResult(Capture(start.capture.origin()), false);
}

MatchResult ENFA::Match(const CharArray & text) const {
    Thread t = {0, start_, Capture(text)};
    return MatchWhen(
        t,
        [](const Thread & current) -> bool { return current.state->IsFinal(); },
        true);
}

} // namespace v2
