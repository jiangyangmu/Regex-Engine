#include "stdafx.h"

#include "Enfa.h"

#include <functional>
#include <iomanip>
#include <sstream>

template <typename T>
std::wstring ToHexString(T i) {
    std::wstringstream stream;
    stream << "0x" << std::setfill(L'0') << std::setw(sizeof(T) * 2) << std::hex
           << i;
    return stream.str();
}

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
            v.insert(v.end(), s->edge.out_.rbegin(), s->edge.out_.rend());
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
        if (kv.first->Tags().HasCaptureTag())
            o += std::wstring(L"\t") +
                (kv.first->Tags().GetCaptureTag().is_begin ? L"B" : L"E") +
                std::to_wstring(kv.first->Tags().GetCaptureTag().capture_id);
        if (kv.first->Tags().HasLookAroundTag())
            o += std::wstring(L"\t") +
                (kv.first->Tags().GetLookAroundTag().is_begin ? L"LB" : L"LE") +
                std::to_wstring(kv.first->Tags().GetLookAroundTag().id);
        if (kv.first->Tags().HasAtomicTag())
            o += std::wstring(L"\t") +
                (kv.first->Tags().GetAtomicTag().is_begin ? L"AB" : L"AE") +
                std::to_wstring(kv.first->Tags().GetAtomicTag().atomic_id);
        if (kv.first->Tags().HasRepeatTag())
            o += std::wstring(L"\t") + L"{" +
                std::to_wstring(kv.first->Tags().GetRepeatTag().min) + L"," +
                (kv.first->Tags().GetRepeatTag().has_max
                     ? std::to_wstring(kv.first->Tags().GetRepeatTag().max)
                     : L"") +
                L"}";
    }

    std::map<int, const EnfaState *> mm;
    for (auto kv : m)
        mm[kv.second] = kv.first;

    CharArray s;
    for (auto kv : mm)
        s += ToHexString(kv.second) + L"\t[" + std::to_wstring(kv.first) +
            L"] " + out[kv.first] + L"\n";
    return s;
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Char(::Char::Type c) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();
    in->edge = {
        EnfaState::CHAR_OUT,
        c,
        0,
    };
    in->edge.out_.push_back(out);
    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::BackReference(
    size_t ref_capture_id) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();
    in->edge = {
        EnfaState::BACKREF_OUT,
        0,
        ref_capture_id,
    };
    in->edge.out_.push_back(out);
    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Alter(StatePort sp1,
                                                    StatePort sp2) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    in->edge.out_.push_back(sp1.in);
    in->edge.out_.push_back(sp2.in);

    sp1.out->edge = sp2.out->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp1.out->edge.out_.push_back(out);
    sp2.out->edge.out_.push_back(out);

    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Repeat(StatePort sp,
                                                     struct Repeat rep) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp.out->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    in->edge.out_.push_back(sp.in);
    if (rep.min == 0)
        in->edge.out_.push_back(out);

    // Match algorithm depends on push order here.
    sp.out->edge.out_.push_back(sp.in);
    sp.out->edge.out_.push_back(out);

    sp.out->Tags().SetRepeatTag({rep.repeat_id, rep.min, rep.max, rep.has_max});

    return {in, out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Concat(StatePort sp1,
                                                     StatePort sp2) {
    sp1.out->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    sp1.out->edge.out_.push_back(sp2.in);

    return {sp1.in, sp2.out};
}

EnfaStateBuilder::StatePort EnfaStateBuilder::Group(StatePort sp) {
    EnfaState * in = new EnfaState();
    EnfaState * out = new EnfaState();

    in->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };
    sp.out->edge = {
        EnfaState::EPSILON_OUT,
        0,
        0,
    };

    in->edge.out_.push_back(sp.in);
    sp.out->edge.out_.push_back(out);

    return {in, out};
}
