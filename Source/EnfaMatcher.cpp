#include "stdafx.h"

#include <functional>

#include "CharMatcher.h"
#include "EnfaMatcher.h"
#include "RegexCompiler.h"

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
    LexMatcher input;
    const EnfaState * state;
    Capture capture;
    std::vector<std::pair<int, size_t>> id_to_repcnt;
};

MatchResult MatchWhen(const Thread & start,
                      std::function<bool(const Thread &)> pred,
                      bool forward_match) {
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
        std::wcout << L"--------------------------------------" << std::endl;
        std::wcout << debug_indent;
        std::wcout << t.input.Origin() << std::endl;
        std::wcout << debug_indent << std::wstring(t.input.CurrentPos(), ' ')
                   << '^' << std::endl;
#endif

        if (t.state->Tags().HasCaptureTag())
        {
            t.capture.DoCapture(t.state->Tags().GetCaptureTag().capture_id,
                                t.input.CurrentPos(),
                                t.state->Tags().GetCaptureTag().is_begin);
        }

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
        if (t.state->Tags().HasLookAroundTag() &&
            t.state->Tags().GetLookAroundTag().is_begin)
        {
#ifdef DEBUG
            std::wcout << L"Eval "
                       << (t.state->Tags().GetLookAroundTag().is_forward
                               ? L"lookahead "
                               : L"lookbehind ")
                       << t.state->Tags().GetLookAroundTag().id << L" at "
                       << t.input.CurrentPos() << std::endl;
            debug_indent += L"  ";
#endif

            int lookaround_id = t.state->Tags().GetLookAroundTag().id;
            bool is_forward = t.state->Tags().GetLookAroundTag().is_forward;
            Thread start = {t.input, t.state->Out(), t.capture};
#ifdef DEBUG_STATS
            stat_save.push_back(generated_threads);
            stat_save.push_back(processed_threads);
            generated_threads = processed_threads = 0;
#endif
            if (MatchWhen(start,
                          [lookaround_id,
                           &t](const Thread & current) mutable -> bool {
                              bool good =
                                  current.state->Tags().HasLookAroundTag() &&
                                  !current.state->Tags()
                                       .GetLookAroundTag()
                                       .is_begin &&
                                  current.state->Tags().GetLookAroundTag().id ==
                                      lookaround_id;
                              if (good)
                                  t.state = current.state;
                              return good;
                          },
                          is_forward)
                    .Matched())
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
        else if (t.state->Tags().HasAtomicTag() &&
                 t.state->Tags().GetAtomicTag().is_begin)
        {
#ifdef DEBUG
            std::wcout << "Eval atomic "
                       << t.state->Tags().GetAtomicTag().atomic_id << " at "
                       << t.input.CurrentPos() << std::endl;
            debug_indent += L"  ";
#endif

            int atomic_id = t.state->Tags().GetAtomicTag().atomic_id;
            Thread start = {t.input, t.state->Out(), t.capture};
#ifdef DEBUG_STATS
            stat_save.push_back(generated_threads);
            stat_save.push_back(processed_threads);
            generated_threads = processed_threads = 0;
#endif
            if (MatchWhen(
                    start,
                    [atomic_id, &t](const Thread & current) mutable -> bool {
                        bool good = current.state->Tags().HasAtomicTag() &&
                            !current.state->Tags().GetAtomicTag().is_begin &&
                            current.state->Tags().GetAtomicTag().atomic_id ==
                                atomic_id;
                        if (good)
                        {
                            t.input = current.input;
                            t.state = current.state;
                            current.capture.CopyTo(t.capture);
                        }
                        return good;
                    },
                    true)
                    .Matched())
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
            bool matched = forward_match
                ? t.input.Match(t.state->Char())
                : t.input.MatchBackword(t.state->Char());
            if (matched)
            {
                T.push_back(
                    {t.input, t.state->Out(), t.capture, t.id_to_repcnt});
            }
        }
        else if (t.state->IsBackReference())
        {
            RView text = t.capture.Group(t.state->BackReference()).GetLast();
            bool matched = forward_match ? t.input.MatchRange(text)
                                         : t.input.MatchRangeBackword(text);
            if (matched)
            {
                T.push_back(
                    {t.input, t.state->Out(), t.capture, t.id_to_repcnt});
            }
        }
        else if (t.state->IsEpsilon())
        {
            if (t.state->Tags().HasRepeatTag())
            {
                auto repeat = t.state->Tags().GetRepeatTag();
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
                {
                    T.push_back({t.input, outs[0], t.capture, t.id_to_repcnt});
                }
                else if (!repeat.has_max || current < repeat.max)
                {
                    T.push_back({t.input, outs[1], t.capture, t.id_to_repcnt});
                    T.back().id_to_repcnt.pop_back();
                    T.push_back({t.input, outs[0], t.capture, t.id_to_repcnt});
                }
                else
                {
                    T.push_back({t.input, outs[1], t.capture, t.id_to_repcnt});
                    T.back().id_to_repcnt.pop_back();
                }
            }
            else
            {
                for (auto out = t.state->MultipleOut().rbegin();
                     out != t.state->MultipleOut().rend();
                     ++out)
                {
                    T.push_back({t.input, *out, t.capture, t.id_to_repcnt});
                }
            }
        }
#ifdef DEBUG
        std::wcout << L"[ ";
        for (auto ti = T.rbegin(); ti != T.rend(); ++ti)
            std::wcout << debug[ti->state] << L',';
        std::wcout << L" ]" << std::endl;
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
    return MatchResult(Capture(start.capture.Origin()), false);
}

MatchResult EnfaMatcher::Match(RView text) const {
    Thread t = {LexMatcher(text), start_, Capture(text)};
    return MatchWhen(
        t,
        [](const Thread & current) -> bool { return current.state->IsFinal(); },
        true);
}

std::vector<MatchResult> EnfaMatcher::MatchAll(RView text) const {
    std::vector<MatchResult> matches;

    RView match_text = text;
    while (!match_text.empty())
    {
        MatchResult m = Match(match_text);
        size_t scanned = 1;
        if (m.Matched())
        {
            matches.push_back(m);
            auto range = m.GetCapture().Group(0).GetLastRange();
            scanned = std::max(scanned, range.second - range.first);
        }
        match_text.PopFront(scanned);
    }

    return matches;
}
