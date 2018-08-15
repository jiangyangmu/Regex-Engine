#include "stdafx.h"

#include "Postfix.h"
#include "RegexCompiler.h"
#include "RegexSyntax.h"
#include "util.h"

EnfaState * PostfixToEnfa(std::vector<PostfixNode> & nl) {
    using StatePort = EnfaStateBuilder::StatePort;

    std::vector<StatePort> st;

#define PUSH(x) st.emplace_back(x)
#define POP() st.back(), st.pop_back()

    std::deque<StatePort> lookbehinds;
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
                PUSH(EnfaStateBuilder::Char(n.chr));
                break;
            case PostfixNode::BACKREF_INPUT:
                PUSH(EnfaStateBuilder::BackReference(n.backref.capture_id));
                break;
            case PostfixNode::REPEAT:
                sp = POP();
                PUSH(EnfaStateBuilder::Repeat(sp, n.repeat));
                break;
            case PostfixNode::CONCAT:
                sp2 = POP();
                sp1 = POP();
                PUSH(EnfaStateBuilder::Concat(sp1, sp2));
                break;
            case PostfixNode::ALTER:
                sp2 = POP();
                sp1 = POP();
                PUSH(EnfaStateBuilder::Alter(sp1, sp2));
                break;
            case PostfixNode::GROUP:
                sp = POP();
                sp = EnfaStateBuilder::Group(sp);
                if (n.group.type == Group::CAPTURE)
                {
                    sp.in->Tags().SetCaptureTag({n.group.capture_id, true});
                    sp.out->Tags().SetCaptureTag({n.group.capture_id, false});
                }
                else if (n.group.type == Group::LOOK_AHEAD)
                {
                    sp.in->Tags().SetLookAroundTag(
                        {n.group.lookaround_id, true, true});
                    sp.out->Tags().SetLookAroundTag(
                        {n.group.lookaround_id, false, true});
                }
                else if (n.group.type == Group::LOOK_BEHIND)
                {
                    sp.in->Tags().SetLookAroundTag(
                        {n.group.lookaround_id, true, false});
                    sp.out->Tags().SetLookAroundTag(
                        {n.group.lookaround_id, false, false});
                }
                else if (n.group.type == Group::NON_CAPTURE)
                {}
                else if (n.group.type == Group::ATOMIC)
                {
                    sp.in->Tags().SetAtomicTag({n.group.atomic_id, true});
                    sp.out->Tags().SetAtomicTag({n.group.atomic_id, false});
                }
                else
                    assert(false);
                PUSH(sp);
                break;
            default:
                break;
        }
    }

    StatePort sp;
    sp = POP();
    assert(st.empty());
    sp.out->SetFinal();

#undef POP
#undef PUSH

    return sp.in;
}

EnfaMatcher RegexCompiler::CompileToEnfa(RString regex) {
    EnfaState * start;
    {
#ifdef DEBUG
        std::wcout << L"pattern: " << regex << std::endl;
#endif
        std::vector<PostfixNode> nl = ParseToPostfix(regex.data());
#ifdef DEBUG
        for (auto n : nl)
            std::wcout << n.DebugString() << " ";
        std::wcout << std::endl;
#endif
        nl = FlipPostfix(nl);
#ifdef DEBUG
        for (auto n : nl)
            std::wcout << n.DebugString() << " ";
        std::wcout << std::endl;
#endif

        start = PostfixToEnfa(nl);
#ifdef DEBUG
        std::wcout << EnfaState::DebugString(start) << std::endl;
#endif
    }

    EnfaMatcher enfa;
    enfa.start_ = start;
    return enfa;
}
