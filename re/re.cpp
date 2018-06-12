#include "stdafx.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// string to RE to epsilon-NFA

// alphabet: [a-z]
//
// char: ()-c->()
//
// concatenation: ()-e->()
//
//                 +->()
// branching: ()-e-+->()
//                 +->()
//
// closure: --+--()-+-->
//            ^--e--+

// (aaa)
// (    -> orb(), andb()
// a    -> s(), s(), t('a')
// a    -> s(), s(), t('a')
// )    -> ande(), ore()

// (a|a|a)
// (    -> orb(), andb()
// a    -> s(), s(), t('a')
// |    -> ande(), andb()
// a    -> s(), s(), t('a')
// |    -> ande(), andb()
// a    -> s(), s(), t('a')
// )    -> ande(), ore()

// (ab*a)
// (    -> orb(), andb()
// a    -> s(), s(), t('a')
// b    -> s(), s(), t('b')
// *    -> c()
// a    -> s(), s(), t('a')
// )    -> ande(), ore()

// for debug.
static int idgen = 0;

class State {
    int id;
    bool is_terminal;
    std::map<char, std::set<State *>> transition;
    std::set<State *> eclosure;

public:
    State()
        : id(idgen++)
        , is_terminal(false) {
    }
    void SetTerminal() {
        is_terminal = true;
    }
    bool IsTerminal() const {
        return is_terminal;
    }
    void AddTransition(char c, State * s) {
        transition[c].insert(s);
    }
    void ComputeEClosure() {
        // assume no loop.
        if (eclosure.empty() && !transition[0].empty())
        {
            eclosure.insert(transition[0].begin(), transition[0].end());
            for (State * s : transition[0])
            {
                s->ComputeEClosure();
                eclosure.insert(s->eclosure.begin(), s->eclosure.end());
            }
        }
    }
    std::set<State *> GetTransition(char c) {
        assert(c != 0);

        std::set<State *> from = {this};
        ComputeEClosure();
        from.insert(eclosure.begin(), eclosure.end());

        std::set<State *> to;
        for (State * s : from)
        {
            to.insert(s->transition[c].begin(), s->transition[c].end());
        }

        std::set<State *> to_closure = to;
        for (State * s : to)
        {
            s->ComputeEClosure();
            to_closure.insert(s->eclosure.begin(), s->eclosure.end());
        }
        return to_closure;
    }
};

struct StatePort {
    State * in;
    State * out;
};

class ENFABuilder {
    std::vector<StatePort> ports_;
    std::vector<size_t> pos_;

public:
    void S() {
        State * a = new State();
        ports_.push_back({a, a});
    }
    void T(char c) {
        StatePort s2 = ports_.back();
        ports_.pop_back();
        StatePort s1 = ports_.back();
        ports_.pop_back();
        s1.out->AddTransition(c, s2.in);
        ports_.push_back({s1.in, s2.out});
    }
    void C() {
        State * a = new State();
        StatePort sp = ports_.back();
        ports_.pop_back();
        sp.out->AddTransition(0, sp.in);
        sp.in->AddTransition(0, a);
        ports_.push_back({sp.in, a});
    }
    void OrB() {
        pos_.push_back(ports_.size());
    }
    void OrE() {
        State * a = new State();
        State * b = new State();
        for (size_t i = pos_.back(); i < ports_.size(); ++i)
        {
            a->AddTransition(0, ports_[i].in);
            ports_[i].out->AddTransition(0, b);
        }
        ports_.erase(ports_.begin() + pos_.back(), ports_.end());
        ports_.push_back({a, b});
        pos_.pop_back();
    }
    void AndB() {
        pos_.push_back(ports_.size());
    }
    void AndE() {
        State * a = ports_[pos_.back()].in;
        State * b = ports_.back().out;
        for (size_t i = pos_.back(); i < ports_.size() - 1; ++i)
        {
            ports_[i].out->AddTransition(0, ports_[i + 1].in);
        }
        ports_.erase(ports_.begin() + pos_.back(), ports_.end());
        ports_.push_back({a, b});
        pos_.pop_back();
    }
    static StatePort Compile(std::string re) {
        ENFABuilder b;

        for (char c : re)
        {
            switch (c)
            {
                case '(':
                    b.OrB();
                    b.AndB();
                    break;
                case '|':
                    b.AndE();
                    b.AndB();
                    break;
                case ')':
                    b.AndE();
                    b.OrE();
                    break;
                case '*':
                    b.C();
                    break;
                default:
                    assert(c >= 'a' && c <= 'z');
                    b.S();
                    b.S();
                    b.T(c);
                    break;
            }
        }

        assert(b.ports_.size() == 1 && b.pos_.empty());
        StatePort sp = b.ports_.back();
        sp.out->SetTerminal();
        return sp;
    }
};

bool Match(StatePort enfa, std::string str) {
    std::set<State *> set1 = {enfa.in};
    std::set<State *> set2;

    for (char c : str)
    {
        for (State * s : set1)
        {
            auto transition = s->GetTransition(c);
            set2.insert(transition.begin(), transition.end());
        }
        set2.swap(set1);
        set2.clear();
    }

    return std::find_if(set1.begin(), set1.end(), [](State * s) -> bool {
               return s->IsTerminal();
           }) != set1.end();
}

int main() {
    std::string regex = "(ab*c|de*f)";
    StatePort enfa = ENFABuilder::Compile(regex);

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        std::cout << (Match(enfa, line) ? "ok." : "reject.") << std::endl;
    }

    return 0;
}
