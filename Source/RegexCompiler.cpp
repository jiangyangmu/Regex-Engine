#include "stdafx.h"

#include "RegexCompiler.h"
#include "regex.h"
#include "util.h"
#include "Postfix.h"

using namespace v2;

EnfaState * PostfixToEnfa(std::vector<PostfixNode> & nl) {
    using StatePort = EnfaStateBuilder::StatePort;

    std::vector<StatePort> st;

#define push(x) st.emplace_back(x)
#define pop() st.back(), st.pop_back()

    for (PostfixNode & n : nl)
    {
        StatePort sp, sp1, sp2;
        switch (n.type)
        {
            case PostfixNode::NULL_INPUT:
                // TODO: support null branch
                assert(false);
                break;
            case PostfixNode::CHAR_INPUT:
                push(EnfaStateBuilder::Char(n.chr.c));
                break;
            case PostfixNode::BACKREF_INPUT:
                push(EnfaStateBuilder::BackReference(n.backref.capture_id));
                break;
            case PostfixNode::REPEAT:
                sp = pop();
                push(EnfaStateBuilder::Repeat(sp, n.repeat));
                break;
            case PostfixNode::CONCAT:
                sp2 = pop();
                sp1 = pop();
                push(EnfaStateBuilder::Concat(sp1, sp2));
                break;
            case PostfixNode::ALTER:
                sp2 = pop();
                sp1 = pop();
                push(EnfaStateBuilder::Alter(sp1, sp2));
                break;
            case PostfixNode::GROUP:
                sp = pop();
                if (n.group.type == Group::LOOK_BEHIND)
                    sp = EnfaStateBuilder::InverseGroup(sp);
                else
                    sp = EnfaStateBuilder::Group(sp);
                if (n.group.type == Group::CAPTURE)
                {
                    sp.in->SetCaptureTag({n.group.capture_id, true});
                    sp.out->SetCaptureTag({n.group.capture_id, false});
                }
                else if (n.group.type == Group::LOOK_AHEAD)
                {
                    sp.in->SetLookAroundTag(
                        {n.group.lookaround_id, true, true});
                    sp.out->SetLookAroundTag(
                        {n.group.lookaround_id, false, true});
                }
                else if (n.group.type == Group::LOOK_BEHIND)
                {
                    sp.in->SetLookAroundTag(
                        {n.group.lookaround_id, true, false});
                    sp.out->SetLookAroundTag(
                        {n.group.lookaround_id, false, false});
                }
                else if (n.group.type == Group::NON_CAPTURE)
                {}
                else if (n.group.type == Group::ATOMIC)
                {
                    sp.in->SetAtomicTag({n.group.atomic_id, true});
                    sp.out->SetAtomicTag({n.group.atomic_id, false});
                }
                else
                    assert(false);
                push(sp);
                break;
            default:
                break;
        }
    }

    StatePort sp;
    sp = pop();
    assert(st.empty());
    sp.out->SetFinal();

#undef pop
#undef push

    return sp.in;
}

v2::EnfaMatcher RegexCompiler::CompileToEnfa(CharArray regex) {
    EnfaState * start;
    {
#ifdef DEBUG
        std::wcout << L"pattern: " << regex << std::endl;
#endif
        std::vector<PostfixNode> nl = ParseToPostfix(regex.data());
#ifdef DEBUG
        for (auto n : nl)
        {
            std::wcout << n.DebugString() << " ";
        }
        std::wcout << std::endl;
#endif

        start = PostfixToEnfa(nl);
#ifdef DEBUG
        std::wcout << EnfaState::DebugString(start) << std::endl;
#endif
    }

    v2::EnfaMatcher enfa;
    enfa.start_ = start;
    return enfa;
}
