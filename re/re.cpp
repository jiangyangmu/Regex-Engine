#include "stdafx.h"

#include <algorithm>
#include <cassert>
#include <deque>
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
    int id_;
    bool is_terminal;
    std::map<char, std::set<State *>> transition;
    std::set<State *> eclosure;

public:
    State()
        : id_(idgen++)
        , is_terminal(false) {
    }
    int id() const {
        return id_;
    }
    const std::set<State *> & closure() {
        // assume no loop.
        if (eclosure.empty())
        {
            eclosure.insert(this);
            eclosure.insert(transition[0].begin(), transition[0].end());
            for (State * s : transition[0])
            {
                eclosure.insert(s->closure().begin(), s->closure().end());
            }
        }
        return eclosure;
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

    std::set<State *> GetTransition(char c) {
        assert(c != 0);

        std::set<State *> from = {this};
        from.insert(closure().begin(), closure().end());

        std::set<State *> to;
        for (State * s : from)
        {
            to.insert(s->transition[c].begin(), s->transition[c].end());
        }

        std::set<State *> to_closure = to;
        for (State * s : to)
        {
            to_closure.insert(s->closure().begin(), s->closure().end());
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
    static std::vector<int> Compile(StatePort enfa) {
        // int[][26]
        std::vector<int> dfa(128 * 26 + 128, -1);

        struct H {
            uint64_t a, b;

            H(const std::set<State *> & states)
                : a(0)
                , b(0) {
                for (State * s : states)
                {
                    assert(s->id() >= 0 && s->id() < 128);
                    (s->id() < 64 ? a : b) |= ((uint64_t)1 << s->id());
                }
            };
            bool operator<(const H & h) const {
                return b < h.b || (b == h.b && a < h.a);
            }
        };

        std::deque<std::set<State *>> q = {enfa.in->closure()};
        std::set<State *> s1, s2;

        std::map<H, int> sidmap;
        int sid = 0;
        sidmap[H(q.front())] = sid++;
        while (!q.empty())
        {
            s1 = q.front();
            q.pop_front();

            int * is_terminal = &dfa[dfa.size() - 128 + sidmap[H(s1)]];
            for (State * s : s1)
            {
                if (s->IsTerminal())
                {
                    *is_terminal = 0;
                    break;
                }
            }

            int * row = dfa.data() + 26 * sidmap[H(s1)];
            for (char c = 'a'; c <= 'z'; ++c)
            {
                s2.clear();
                for (State * s : s1)
                {
                    auto transition = s->GetTransition(c);
                    s2.insert(transition.begin(), transition.end());
                }
                if (s2.empty())
                    continue;

                if (sidmap.find(H(s2)) == sidmap.end())
                {
                    sidmap[H(s2)] = sid++;
                    assert(sid <= 128);
                    q.push_back(s2);
                }
                int col = c - 'a';
                row[col] = sidmap[H(s2)];
            }
        }

        return dfa;
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

bool Match(const std::vector<int> & dfa, std::string str) {
    int s = 0;
    for (char c : str)
    {
        s = dfa[s * 26 + (c - 'a')];
        if (s == -1)
        {
            return false;
        }
    }
    return dfa[dfa.size() - 128 + s] == 0;
}

void Print(const std::vector<int> & dfa) {
    for (size_t i = 0; i < 128; ++i)
    {
        std::string s;
        for (int c = 0; c < 26; ++c)
        {
            if (dfa[i * 26 + c] != -1)
            {
                s += ' ';
                s += c + 'a';
                s += "->";
                s += std::to_string(dfa[i * 26 + c]);
            }
        }
        if (dfa[dfa.size() - 128 + i] == 0)
        {
            s += " [terminal]";
        }
        if (!s.empty())
        {
            std::cout << i << ": " << s << std::endl;
        }
    }
}

int main() {
    std::string regex = "(ab*c|de*f)";
    StatePort enfa = ENFABuilder::Compile(regex);
    std::vector<int> dfa = ENFABuilder::Compile(enfa);

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        std::cout << "ENFA: " << (Match(enfa, line) ? "ok." : "reject.")
            << std::endl;
        std::cout << "DFA:  " << (Match(dfa, line) ? "ok." : "reject.")
                  << std::endl;
    }

    return 0;
}
