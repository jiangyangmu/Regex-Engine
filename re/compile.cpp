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

    std::string DebugString() const {
        std::string s;
        switch (type)
        {
            case NULL_INPUT:
                s += "INPUT()";
                break;
            case CHAR_INPUT:
                s += "INPUT(";
                s += chr.c;
                s += ")";
                break;
            case BACKREF_INPUT:
                s += "BACKREF(" + std::to_string(backref.capture_id) + ")";
                break;
            case REPEAT:
                s += "REPEAT(" + std::to_string(repeat.min) + "," +
                    (repeat.has_max ? std::to_string(repeat.max) : "") + ")";
                break;
            case CONCAT:
                s += "CONCAT";
                break;
            case ALTER:
                s += "ALTER";
                break;
            case GROUP:
                switch (group.type)
                {
                    case Group::CAPTURE:
                        s +=
                            "CAPTURE(" + std::to_string(group.capture_id) + ")";
                        break;
                    case Group::NON_CAPTURE:
                        s += "NON_CAPTURE";
                        break;
                    case Group::LOOK_AHEAD:
                        s += "LOOKAHEAD";
                        break;
                    case Group::LOOK_BEHIND:
                        s += "LOOKBEHIND";
                        break;
                    case Group::ATOMIC:
                        s += "ATOMIC";
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
    explicit RegexParser(const char * regex)
        : capture_id_gen_(0)
        , lookaround_id_gen_(0)
        , atomic_id_gen_(0)
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
            Repeat r = {0, 0, false};
            nl_.emplace_back(r);
        }
        else if (*regex_ == '{')
        {
            ++regex_;
            Repeat r = {0, 0, false};

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
        if (*regex_ >= 'a' && *regex_ <= 'z')
        {
            Char c = {*regex_++};
            nl_.emplace_back(c);
            repeat();
            return true;
        }
        else if (*regex_ == '\\')
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
    const char * regex_;
    NodeList nl_;
};

/*
 * regex string to postfix
 */

NodeList RegexToPostfix(const char * regex) {
    return RegexParser(regex).get();
}

namespace v1 {

/*
 * postfix to epsilon-NFA
 */

// epsilon-NFA state-set port
struct StatePort {
    EnfaState * in; // for read
    std::set<EnfaState **> out; // for write

    // capturing group tag
    CaptureTag tag;
};

EnfaState * PostfixToEnfa(NodeList & nl) {
    std::vector<StatePort> st;

#define push(x) st.emplace_back(std::move(x))
#define pop() st.back(), st.pop_back()
#define patch(out, tag, in)             \
    for (auto __o : (out))              \
    {                                   \
        *__o = (in);                    \
        (in)->MutableTag()->Merge(tag); \
    }

    for (Node & n : nl)
    {
        EnfaState * s;
        StatePort sp, sp1, sp2;
        switch (n.type)
        {
            case Node::NULL_INPUT:
                // TODO: support null branch
                assert(false);
                break;
            case Node::CHAR_INPUT:
                s = EnfaStateBuilder::NewCharState(n.chr.c);
                sp.in = s;
                sp.out.insert(s->MutableOut());
                push(sp);
                break;
            case Node::BACKREF_INPUT:
                s = EnfaStateBuilder::NewBackReferenceState(
                    n.backref.capture_id);
                sp.in = s;
                sp.out.insert(s->MutableOut());
                push(sp);
                break;
            case Node::REPEAT:
                sp1 = pop();
                s = EnfaStateBuilder::NewSplitState(sp1.in, nullptr);
                patch(sp1.out, sp1.tag, s);

                sp.in = s;
                sp.out.insert(s->MutableOut1());
                push(sp);
                break;
            case Node::CONCAT:
                sp2 = pop();
                sp1 = pop();
                patch(sp1.out, sp1.tag, sp2.in);

                sp.in = sp1.in;
                sp.out = sp2.out;
                sp.tag = sp2.tag;
                push(sp);
                break;
            case Node::ALTER:
                sp2 = pop();
                sp1 = pop();

                s = EnfaStateBuilder::NewSplitState(sp1.in, sp2.in);

                sp.in = s;
                sp.out = sp1.out;
                sp.out.insert(sp2.out.begin(), sp2.out.end());
                sp.tag = sp1.tag;
                sp.tag.Merge(sp2.tag);
                push(sp);
                break;
            case Node::GROUP:
                if (n.group.type == Group::CAPTURE)
                {
                    sp = pop();
                    sp.in->MutableTag()->AddBegin(n.group.capture_id);
                    sp.tag.AddEnd(n.group.capture_id);
                    push(sp);
                }
                break;
            default:
                break;
        }
    }

    StatePort sp;
    EnfaState * s;
    sp = pop();
    assert(st.empty());
    s = EnfaStateBuilder::NewFinalState();
    patch(sp.out, sp.tag, s);

#undef patch
#undef pop
#undef push

    return sp.in;
}

EnfaState * RegexToEnfa(const std::string & regex) {
#ifdef DEBUG
    std::cout << "pattern: " << regex << std::endl;
#endif
    NodeList nl = RegexToPostfix(regex.data());
#ifdef DEBUG
    for (auto n : nl)
    {
        std::cout << n.DebugString() << " ";
    }
    std::cout << std::endl;
#endif

    EnfaState * start = PostfixToEnfa(nl);
#ifdef DEBUG
    std::cout << EnfaState::DebugString(start) << std::endl;
#endif
    return start;
}

} // namespace v1

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
                push(EnfaStateBuilder::Repeat(sp));
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

EnfaState * RegexToEnfa(const std::string & regex) {
#ifdef DEBUG
    std::cout << "pattern: " << regex << std::endl;
#endif
    NodeList nl = RegexToPostfix(regex.data());
#ifdef DEBUG
    for (auto n : nl)
    {
        std::cout << n.DebugString() << " ";
    }
    std::cout << std::endl;
#endif

    EnfaState * start = PostfixToEnfa(nl);
#ifdef DEBUG
    std::cout << EnfaState::DebugString(start) << std::endl;
#endif
    return start;
}

} // namespace v2

v1::ENFA FABuilder::CompileV1(std::string regex) {
    v1::ENFA enfa;
    enfa.start_ = v1::RegexToEnfa(regex);
    return enfa;
}

v2::ENFA FABuilder::CompileV2(std::string regex) {
    v2::ENFA enfa;
    enfa.start_ = v2::RegexToEnfa(regex);
    return enfa;
}
