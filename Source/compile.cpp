#include "stdafx.h"

#include "compile.h"
#include "regex.h"
#include "util.h"

// postfix node
struct Node {
    enum Type {
        INVALID,
        NULL_INPUT, // empty group or alternation
        CHAR_INPUT,
        BACKREF_INPUT,
        REPEAT,
        CONCAT,
        ALTER,
        GROUP,
    };

    explicit Node(Char c)
        : type(CHAR_INPUT)
        , chr(c) {
    }
    explicit Node(Group g)
        : type(GROUP)
        , group(g) {
    }
    explicit Node(BackReference br)
        : type(BACKREF_INPUT)
        , backref(br) {
    }
    explicit Node(Repeat rep)
        : type(REPEAT)
        , repeat(rep) {
    }
    explicit Node(Type t)
        : type(t) {
        assert(type == CONCAT || type == ALTER || type == NULL_INPUT);
    }

    CharArray DebugString() const {
        CharArray s;
        switch (type)
        {
            case NULL_INPUT:
                s += L"INPUT()";
                break;
            case CHAR_INPUT:
                s += L"INPUT(";
                s += chr.c;
                s += L")";
                break;
            case BACKREF_INPUT:
                s += L"BACKREF(" + std::to_wstring(backref.capture_id) + L")";
                break;
            case REPEAT:
                s += L"REPEAT(" + std::to_wstring(repeat.min) + L"," +
                    (repeat.has_max ? std::to_wstring(repeat.max) : L"") + L")";
                break;
            case CONCAT:
                s += L"CONCAT";
                break;
            case ALTER:
                s += L"ALTER";
                break;
            case GROUP:
                switch (group.type)
                {
                    case Group::CAPTURE:
                        s += L"CAPTURE(" + std::to_wstring(group.capture_id) +
                            L")";
                        break;
                    case Group::NON_CAPTURE:
                        s += L"NON_CAPTURE";
                        break;
                    case Group::LOOK_AHEAD:
                        s += L"LOOKAHEAD";
                        break;
                    case Group::LOOK_BEHIND:
                        s += L"LOOKBEHIND";
                        break;
                    case Group::ATOMIC:
                        s += L"ATOMIC";
                        break;
                }
                break;
            default:
                break;
        }
        return s;
    }

    Type type;
    union {
        Char chr;
        Group group;
        BackReference backref;
        Repeat repeat;
    };
};
typedef std::vector<Node> NodeList;

class RegexParser {
public:
    explicit RegexParser(const Char::Type * regex)
        : capture_id_gen_(0)
        , lookaround_id_gen_(0)
        , atomic_id_gen_(0)
        , repeat_id_gen_(0)
        , regex_(regex) {
        group();
    }
    NodeList get() {
        return nl_;
    }

private:
    // TODO: check regex_ boundary.
    void repeat() {
        if (*regex_ == '*')
        {
            ++regex_;
            Repeat r = {repeat_id_gen_++, 0, 0, false};
            nl_.emplace_back(r);
        }
        else if (*regex_ == '{')
        {
            ++regex_;
            Repeat r = {repeat_id_gen_++, 0, 0, false};

            assert(*regex_ == ',' || IsDigit(*regex_));
            if (IsDigit(*regex_))
                r.min = ParseInt32(&regex_);
            assert(*regex_ == ',');
            ++regex_;
            if (IsDigit(*regex_))
            {
                r.max = ParseInt32(&regex_);
                r.has_max = true;
                assert(r.max >= r.min);
            }

            assert(*regex_ == '}');
            ++regex_;

            nl_.emplace_back(r);
        }
    }
    bool item() {
        if (*regex_ == '\\')
        {
            ++regex_;
            assert(*regex_ >= '0' && *regex_ <= '9');
            BackReference b = {*regex_++ - '0'};
            nl_.emplace_back(b);
            repeat();
            return true;
        }
        else if (*regex_ == '(')
        {
            group();
            repeat();
            return true;
        }
        else if (*regex_ != ')' && *regex_ != '|')
        {
            // TODO: check not special char
            Char c = {*regex_++};
            nl_.emplace_back(c);
            repeat();
            return true;
        }
        return false;
    }
    void concat() {
        assert(item());
        while (item())
            nl_.emplace_back(Node::CONCAT);
    }
    void alter() {
        if (*regex_ == '|' || *regex_ == ')')
        {
            nl_.emplace_back(Node::NULL_INPUT);
            return;
        }

        concat();
        while (*regex_ == '|')
        {
            ++regex_;
            if (*regex_ == '|' || *regex_ == ')')
            {
                nl_.emplace_back(Node::NULL_INPUT);
                continue;
            }
            concat();
            nl_.emplace_back(Node::ALTER);
        }
    }
    void group() {
        assert(*regex_ == '(');
        ++regex_;

        Group g;

        g.type = Group::CAPTURE;
        if (*regex_ == '?' && *(regex_ + 1) == ':')
            g.type = Group::NON_CAPTURE, regex_ += 2;
        else if (*regex_ == '?' && *(regex_ + 1) == '=')
            g.type = Group::LOOK_AHEAD, g.lookaround_id = lookaround_id_gen_++,
            regex_ += 2;
        else if (*regex_ == '?' && *(regex_ + 1) == '<')
            g.type = Group::LOOK_BEHIND, g.lookaround_id = lookaround_id_gen_++,
            regex_ += 2;
        else if (*regex_ == '?' && *(regex_ + 1) == '>')
            g.type = Group::ATOMIC, g.atomic_id = atomic_id_gen_++, regex_ += 2;
        else
            g.capture_id = capture_id_gen_++;

        alter();

        nl_.emplace_back(g);

        assert(*regex_ == ')');
        ++regex_;
    }

    int capture_id_gen_;
    int lookaround_id_gen_;
    int atomic_id_gen_;
    int repeat_id_gen_;
    const Char::Type * regex_;
    NodeList nl_;
};

/*
 * regex string to postfix
 */

NodeList RegexToPostfix(const Char::Type * regex) {
    return RegexParser(regex).get();
}

namespace v2 {

/*
 * postfix to epsilon-NFA
 */

EnfaState * PostfixToEnfa(NodeList & nl) {
    using StatePort = EnfaStateBuilder::StatePort;

    std::vector<StatePort> st;

#define push(x) st.emplace_back(x)
#define pop() st.back(), st.pop_back()

    for (Node & n : nl)
    {
        StatePort sp, sp1, sp2;
        switch (n.type)
        {
            case Node::NULL_INPUT:
                // TODO: support null branch
                assert(false);
                break;
            case Node::CHAR_INPUT:
                push(EnfaStateBuilder::Char(n.chr.c));
                break;
            case Node::BACKREF_INPUT:
                push(EnfaStateBuilder::BackReference(n.backref.capture_id));
                break;
            case Node::REPEAT:
                sp = pop();
                push(EnfaStateBuilder::Repeat(sp, n.repeat));
                break;
            case Node::CONCAT:
                sp2 = pop();
                sp1 = pop();
                push(EnfaStateBuilder::Concat(sp1, sp2));
                break;
            case Node::ALTER:
                sp2 = pop();
                sp1 = pop();
                push(EnfaStateBuilder::Alter(sp1, sp2));
                break;
            case Node::GROUP:
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

EnfaState * RegexToEnfa(const CharArray & regex) {
#ifdef DEBUG
    std::wcout << L"pattern: " << regex << std::endl;
#endif
    NodeList nl = RegexToPostfix(regex.data());
#ifdef DEBUG
    for (auto n : nl)
    {
        std::wcout << n.DebugString() << " ";
    }
    std::wcout << std::endl;
#endif

    EnfaState * start = PostfixToEnfa(nl);
#ifdef DEBUG
    std::wcout << EnfaState::DebugString(start) << std::endl;
#endif
    return start;
}

} // namespace v2

v2::ENFA FABuilder::CompileV2(CharArray regex) {
    v2::ENFA enfa;
    enfa.start_ = v2::RegexToEnfa(regex);
    return enfa;
}
