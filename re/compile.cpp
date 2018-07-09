#include "stdafx.h"

#include "compile.h"

/*
    # regex grammar
    group = '(' alter ')'
    alter = (concat)? ('|' (concat)?)*
    concat = item+
    item = (char | group) repeat
    char = ASCII | '\' [0-9]
    repeat = ('*')?
*/

// postfix node
struct Node {
    enum Type {
        NULL_INPUT,
        CHAR_INPUT,
        BACKREF_INPUT,
        REPEAT,
        CONCAT,
        ALTER,
        GROUP,
    };

    explicit Node(char c)
        : value_(c)
        , type_(CHAR_INPUT) {
    }
    explicit Node(Type t, int v = 0)
        : value_(v)
        , type_(t) {
    }
    int value() const {
        return value_;
    }
    Type type() const {
        return type_;
    }
    std::string DebugString() const {
        std::string s;
        switch (type_)
        {
            case NULL_INPUT:
                s += "INPUT()";
                break;
            case CHAR_INPUT:
                s += "INPUT(";
                s += static_cast<char>(value_);
                s += ")";
                break;
            case BACKREF_INPUT:
                s += "BACKREF(" + std::to_string(value_) + ")";
                break;
            case REPEAT:
                s += "REPEAT";
                break;
            case CONCAT:
                s += "CONCAT";
                break;
            case ALTER:
                s += "ALTER";
                break;
            case GROUP:
                s += "GROUP(" + std::to_string(value_) + ")";
                break;
            default:
                break;
        }
        return s;
    }

    int value_;
    Type type_;
};
typedef std::vector<Node> NodeList;

// epsilon-NFA state-set port
struct StatePort {
    EnfaState * in; // for read
    std::set<EnfaState **> out; // for write

    // capturing group tag
    CaptureTag tag;
};

class RegexParser {
public:
    explicit RegexParser(const char * regex)
        : idgen_(0)
        , regex_(regex) {
        group();
    }
    NodeList get() {
        return nl_;
    }

private:
    void repeat() {
        if (*regex_ == '*')
        {
            nl_.emplace_back(Node::REPEAT);
            ++regex_;
        }
    }
    bool item() {
        if (*regex_ >= 'a' && *regex_ <= 'z')
        {
            nl_.emplace_back(*regex_++);
            repeat();
            return true;
        }
        else if (*regex_ == '\\')
        {
            ++regex_;
            assert(*regex_ >= '0' && *regex_ <= '9');
            nl_.emplace_back(Node::BACKREF_INPUT, *regex_ - '0');
            ++regex_;
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
        int id = idgen_++;
        assert(*regex_ == '(');
        ++regex_;
        alter();
        assert(*regex_ == ')');
        ++regex_;
        nl_.emplace_back(Node::GROUP, id);
    }
    int idgen_;
    const char * regex_;
    NodeList nl_;
};

/*
 * regex string to postfix
 */

NodeList RegexToPostfix(const char * regex) {
    return RegexParser(regex).get();
}

/*
 * postfix to epsilon-NFA
 */

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
        switch (n.type())
        {
            case Node::NULL_INPUT:
                // TODO: support null branch
                assert(false);
                break;
            case Node::CHAR_INPUT:
                s = EnfaStateBuilder::NewCharState(n.value());
                sp.in = s;
                sp.out.insert(s->MutableOut());
                push(sp);
                break;
            case Node::BACKREF_INPUT:
                s = EnfaStateBuilder::NewBackReferenceState(n.value());
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
                sp = pop();
                sp.in->MutableTag()->AddBegin(n.value());
                sp.tag.AddEnd(n.value());
                push(sp);
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
    NodeList nl = RegexToPostfix(regex.data());
    return PostfixToEnfa(nl);
}

EnfaState * RegexToEnfaDebug(const std::string & regex) {
    std::cout << "pattern: " << regex << std::endl;
    NodeList nl = RegexToPostfix(regex.data());
    for (auto n : nl)
    {
        std::cout << n.DebugString() << " ";
    }
    std::cout << std::endl;

    EnfaState * start = PostfixToEnfa(nl);
    std::cout << EnfaState::DebugString(start) << std::endl;
    return start;
}

ENFA FABuilder::Compile(std::string regex) {
    ENFA enfa;
    enfa.start_ = RegexToEnfaDebug(regex);
    return enfa;
}
