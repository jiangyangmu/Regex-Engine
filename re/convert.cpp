#include "stdafx.h"

#include "enfa.h"

/*
    # regex grammar
    group = '(' alter ')'
    alter = (concat)? ('|' (concat)?)*
    concat = item+
    item = (char | group) repeat
    repeat = ('*')?
*/

// postfix node
struct Node {
    enum Type {
        NULL_INPUT,
        CHAR_INPUT,
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
    State * in; // for read
    std::set<State **> out; // for write

    // capturing group tag
    std::deque<int> etags;
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

State * PostfixToEnfa(NodeList & nl) {
    std::vector<StatePort> st;

#define push(x) st.emplace_back(std::move(x))
#define pop() st.back(), st.pop_back()
#define patch(out, etags, in)    \
    for (auto __o : (out))       \
    {                            \
        *__o = (in);             \
        (in)->AddEndTags(etags); \
    }

    for (Node & n : nl)
    {
        State * s;
        StatePort sp, sp1, sp2;
        switch (n.type())
        {
            case Node::NULL_INPUT:
                // TODO: support null branch
                assert(false);
                break;
            case Node::CHAR_INPUT:
                s = State::NewCharState(n.value());
                sp.in = s;
                sp.out.insert(&s->out);
                push(sp);
                break;
            case Node::REPEAT:
                sp1 = pop();
                s = State::NewSplitState(sp1.in, nullptr);
                patch(sp1.out, sp1.etags, s);

                sp.in = s;
                sp.out.insert(&s->out1);
                push(sp);
                break;
            case Node::CONCAT:
                sp2 = pop();
                sp1 = pop();
                patch(sp1.out, sp1.etags, sp2.in);

                sp.in = sp1.in;
                sp.out = sp2.out;
                sp.etags = sp2.etags;
                push(sp);
                break;
            case Node::ALTER:
                sp2 = pop();
                sp1 = pop();

                s = State::NewSplitState(sp1.in, sp2.in);

                sp.in = s;
                sp.out = sp1.out;
                sp.out.insert(sp2.out.begin(), sp2.out.end());
                sp.etags = sp1.etags;
                sp.etags.insert(
                    sp.etags.begin(), sp2.etags.begin(), sp2.etags.end());
                push(sp);
                break;
            case Node::GROUP:
                sp = pop();
                sp.in->AddBeginTag(n.value());
                sp.etags.push_back(n.value());
                push(sp);
                break;
            default:
                break;
        }
    }

    StatePort sp;
    sp = pop();
    assert(st.empty());
    patch(sp.out, sp.etags, State::NewFinalState());

#undef patch
#undef pop
#undef push

    return sp.in;
}

State * RegexToEnfa(const std::string & input) {
    NodeList nl = RegexToPostfix(input.data());
    return PostfixToEnfa(nl);
}

void ConvertDebugPrint(std::string & regex) {
    std::cout << "pattern: " << regex << std::endl;
    NodeList nl = RegexToPostfix(regex.data());
    for (auto n : nl)
    {
        std::cout << n.DebugString() << " ";
    }
    std::cout << std::endl;

    State * enfa = PostfixToEnfa(nl);
    std::cout << State::DebugString(enfa) << std::endl;
}