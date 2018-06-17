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

class Capture {
public:
    enum Type { BEGIN, END };
    struct Action {
        size_t group_id;
        Type type;
    };
    struct Event {
        size_t group_id;
        Type type;
        size_t pos;
    };

public:
    Capture(const std::string & origin)
        : origin_(origin) {
    }

    void AddEvents(const std::vector<Event> & events) {
        std::map<size_t, std::vector<Event>> m;
        for (const Event & e : events)
        {
            m[e.group_id].push_back(e);
            // BE, B, E
            assert(m[e.group_id].size() <= 2);
            if (m[e.group_id].size() == 2 && e.type == BEGIN)
            {
                assert(m[e.group_id][0].type == END);
                std::swap(m[e.group_id][0], m[e.group_id][1]);
            }
        }
        for (const auto & kv : m)
        {
            events_[kv.first].emplace_back(kv.second);
        }
    }

    void Compute() {
        if (!captured_.empty())
            return;

        for (const auto & kv : events_)
        {
            if (kv.second.size() < 2)
                continue;

            // pattern 0 (a)  : B, E
            // pattern 1 (a)* : B,  BE, BE, ...
            // pattern 2 (a*) : BE,  E,  E, ...
            // pattern 3 (a*)*: BE, BE, BE, ...
            int pattern = -1;
#define IS_BE(x) ((x).size() == 2 && (x)[0].type == BEGIN && (x)[1].type == END)
#define IS_B(x) ((x).size() == 1 && (x)[0].type == BEGIN)
#define IS_E(x) ((x).size() == 1 && (x)[0].type == END)
            auto seq = kv.second.cbegin();
            if (IS_B(*seq))
            {
                ++seq;
                if (IS_E(*seq))
                {
                    assert(++seq == kv.second.cend());
                    pattern = 0;
                }
                else
                {
                    assert(IS_BE(*seq));
                    while (++seq != kv.second.cend())
                    {
                        assert(IS_BE(*seq));
                    }
                    pattern = 1;
                }
            }
            else
            {
                assert(IS_BE(*seq));
                ++seq;
                if (IS_E(*seq))
                {
                    while (++seq != kv.second.cend())
                    {
                        assert(IS_E(*seq));
                    }
                    pattern = 2;
                }
                else
                {
                    assert(IS_BE(*seq));
                    while (++seq != kv.second.cend())
                    {
                        assert(IS_BE(*seq));
                    }
                    pattern = 3;
                }
            }
#undef IS_BE
#undef IS_B
#undef IS_E
            assert(pattern != -1);

            size_t group_id = kv.first;
            auto & evv = kv.second;
            size_t b, e;
            switch (pattern)
            {
                case 0: // pattern 0 (a)  : B, E
                    b = evv.front()[0].pos;
                    e = evv.back()[0].pos;
                    captured_[group_id].push_back(origin_.substr(b, e - b));
                    break;
                case 1: // pattern 1 (a)* : B,  BE, BE, ...
                    for (b = 0; b + 1 < evv.size(); ++b)
                    {
                        captured_[group_id].push_back(origin_.substr(
                            evv[b][0].pos, evv[b + 1][0].pos - evv[b][0].pos));
                    }
                    break;
                case 2: // pattern 2 (a*) : BE,  E,  E, ...
                    b = evv.front()[0].pos;
                    e = evv.back()[0].pos;
                    captured_[group_id].push_back(origin_.substr(b, e - b));
                    break;
                case 3: // pattern 3 (a*)*: BE, BE, BE, ...
                    b = evv.front()[0].pos;
                    e = evv.back()[1].pos;
                    captured_[group_id].push_back(origin_.substr(b, e - b));
                    break;
                default:
                    break;
            }
        }
    }

    std::string DebugString() const {
        std::string s;
        for (const auto & evv : events_)
        {
            s += std::to_string(evv.first) + ":";
            for (const auto & ev : evv.second)
            {
                s += " {";
                for (const auto & e : ev)
                {
                    assert(e.group_id == evv.first);
                    s += (e.type == BEGIN ? "B" : "E");
                    s += std::to_string(e.pos);
                    s += ",";
                }
                if (s.back() == ',')
                    s.pop_back();
                s += "}";
            }
            s += "\n";
        }
        for (const auto & captured : captured_)
        {
            s += std::to_string(captured.first) + ":";
            for (const auto & str : captured.second)
            {
                s += " \"";
                s += str;
                s += "\"";
            }
            s += "\n";
        }
        return s;
    }

private:
    std::map<size_t, std::vector<std::vector<Event>>> events_;
    std::map<size_t, std::vector<std::string>> captured_;
    const std::string & origin_;
};

class Char {
public:
    Char()
        : encode_(0) {
    }
    Char(char c)
        : encode_(c) {
        assert(c >= 'a' && c <= 'z');
    }
    operator uint8_t() const {
        return encode_;
    }
    static size_t CharTypeCount() {
        return 26;
    }
    static const Char epsilon;

private:
    uint8_t encode_;
};
const Char Char::epsilon;

class State {
    int id_;
    bool is_terminal_;
    std::map<Char, std::set<State *>> transition_;
    std::set<State *> eclosure_;
    std::vector<Capture::Action> actions_;

public:
    State()
        : id_(idgen++)
        , is_terminal_(false) {
    }
    int id() const {
        return id_;
    }
    const std::set<State *> & closure() {
        if (eclosure_.empty())
        {
            std::set<int> v = {id()};
            std::deque<State *> q = {this};
            while (!q.empty())
            {
                State * s = q.front();
                q.pop_front();
                eclosure_.insert(s);
                for (State * schild : s->transition_[Char::epsilon])
                {
                    if (v.find(schild->id()) == v.end())
                    {
                        q.push_back(schild);
                        v.insert(schild->id());
                    }
                }
            }
        }
        return eclosure_;
    }
    void SetTerminal() {
        is_terminal_ = true;
    }
    bool IsTerminal() const {
        return is_terminal_;
    }
    void AddTransition(Char c, State * s) {
        transition_[c].insert(s);
    }
    std::set<State *> GetTransition(Char c) {
        assert(c != 0);

        std::set<State *> from = {this};
        from.insert(closure().begin(), closure().end());

        std::set<State *> to;
        for (State * s : from)
        {
            to.insert(s->transition_[c].begin(), s->transition_[c].end());
        }

        std::set<State *> to_closure = to;
        for (State * s : to)
        {
            to_closure.insert(s->closure().begin(), s->closure().end());
        }
        return to_closure;
    }
    void AddAction(Capture::Action action) {
        actions_.push_back(action);
    }
    std::vector<Capture::Event> GetEvents(size_t pos) const {
        std::vector<Capture::Event> actions;
        actions.reserve(actions_.size());
        for (const auto & action : actions_)
        {
            actions.push_back({action.group_id, action.type, pos});
        }
        return actions;
    }
};

struct StatePort {
    State * in;
    State * out;
};

class MatchResult {
public:
    MatchResult(const std::string & origin)
        : origin_(origin)
        , matched_(false) {
    }
    void SetMatched() {
        matched_ = true;
    }
    void BeginGroup(size_t group_id, size_t pos) {
        if (groups_.find(group_id) == groups_.end())
        {
            groups_[group_id].first = pos;
            groups_[group_id].second = 0;
        }
        else
        { assert(groups_[group_id].second == pos); }
    }
    void EndGroup(size_t group_id, size_t pos) {
        assert(groups_.find(group_id) != groups_.end());
        groups_[group_id].second = pos;
    }

    bool Matched() const {
        return matched_;
    }
    size_t MaxGroupId() const {
        assert(!groups_.empty());
        return groups_.crbegin()->first;
    }
    std::string Group(size_t i) const {
        assert(matched_);
        return groups_.count(i)
            ? origin_.substr(groups_.at(i).first,
                             groups_.at(i).second - groups_.at(i).first)
            : "";
    }

private:
    const std::string & origin_;
    bool matched_;
    std::map<size_t, std::pair<size_t, size_t>> groups_;
};

class DFA {
    friend class FABuilder;

public:
    MatchResult Match(const std::string & str) const {
        MatchResult result(str);

        int s = 0;
        size_t pos = 0;
        for (char c : str)
        {
            s = GetTransition(s, c);
            if (s == -1)
                return MatchResult(str);
            ++pos;
        }
        if (IsTerminal(s))
            result.SetMatched();
        return result;
    }

    std::string DebugString() const {
        std::string str;
        for (size_t s = 0; s < is_terminal.size(); ++s)
        {
            std::string sc;
            for (char c = 'a'; c <= 'z'; ++c)
            {
                if (GetTransition(s, c) != -1)
                {
                    sc += ' ';
                    sc += c;
                    sc += "->";
                    sc += std::to_string(GetTransition(s, c));
                }
            }
            if (IsTerminal(s))
            {
                sc += " [terminal]";
            }
            if (!sc.empty())
            {
                str += (str.empty() ? "" : "\n");
                str += std::to_string(s) + ":" + sc;
            }
        }
        return str;
    }

private:
    int AddState() {
        transition.insert(transition.end(), Char::CharTypeCount(), -1);
        is_terminal.push_back(false);
        return (int)(is_terminal.size() - 1);
    }
    void SetTransition(int from, Char c, int to) {
        assert(from >= 0 &&
               (from + 1) * Char::CharTypeCount() <= (int)transition.size() &&
               to >= 0 &&
               (to + 1) * Char::CharTypeCount() <= (int)transition.size());
        transition[from * Char::CharTypeCount() + c] = to;
    }
    int GetTransition(int state, char c) const {
        assert(state >= 0 &&
               (state + 1) * Char::CharTypeCount() <= (int)transition.size());
        return transition[state * 26 + (c - 'a')];
    }
    void SetTerminal(int state) {
        assert(state >= 0 &&
               (state + 1) * Char::CharTypeCount() <= (int)transition.size());
        is_terminal[state] = true;
    }
    bool IsTerminal(int state) const {
        return is_terminal[state];
    }

    std::vector<int> transition;
    std::vector<bool> is_terminal;
};

class FABuilder {
    std::vector<StatePort> ports_;
    std::vector<size_t> pos_;
    std::vector<size_t> group_ids_;
    size_t group_idgen_;

public:
    FABuilder()
        : group_idgen_(0) {
    }
    void S() {
        State * a = new State();
        ports_.push_back({a, a});
    }
    void T(Char c) {
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
        sp.out->AddTransition(Char::epsilon, sp.in);
        sp.in->AddTransition(Char::epsilon, a);
        ports_.push_back({sp.in, a});
    }
    void GroupB() {
        group_ids_.push_back(group_idgen_++);
    }
    void GroupE() {
        size_t group_id = group_ids_.back();
        group_ids_.pop_back();

        State * a = new State();
        State * b = new State();
        a->AddAction({group_id, Capture::BEGIN});
        b->AddAction({group_id, Capture::END});
        a->AddTransition(Char::epsilon, ports_.back().in);
        ports_.back().out->AddTransition(Char::epsilon, b);
        ports_.back() = {a, b};
    }
    void OrB() {
        pos_.push_back(ports_.size());
    }
    void OrE() {
        assert(pos_.back() < ports_.size());
        if (ports_.size() - pos_.back() > 1)
        {
            State * a = new State();
            State * b = new State();
            for (size_t i = pos_.back(); i < ports_.size(); ++i)
            {
                a->AddTransition(Char::epsilon, ports_[i].in);
                ports_[i].out->AddTransition(Char::epsilon, b);
            }
            ports_.erase(ports_.begin() + pos_.back(), ports_.end());
            ports_.push_back({a, b});
        }
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
            ports_[i].out->AddTransition(Char::epsilon, ports_[i + 1].in);
        }
        ports_.erase(ports_.begin() + pos_.back(), ports_.end());
        ports_.push_back({a, b});
        pos_.pop_back();
    }
    static StatePort Compile(std::string re) {
        FABuilder b;

        for (char c : re)
        {
            switch (c)
            {
                case '(':
                    b.GroupB();
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
                    b.GroupE();
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
    static DFA Compile(StatePort enfa) {
        DFA dfa;

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
        sidmap[H(q.front())] = dfa.AddState();
        while (!q.empty())
        {
            s1 = q.front(), q.pop_front();

            int from = sidmap[H(s1)];
            for (State * s : s1)
            {
                if (s->IsTerminal())
                {
                    dfa.SetTerminal(from);
                    break;
                }
            }

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
                    sidmap[H(s2)] = dfa.AddState();
                    q.push_back(s2);
                }
                int to = sidmap[H(s2)];
                dfa.SetTransition(from, c, to);
            }
        }

        return dfa;
    }
};

bool Match(StatePort enfa, std::string str) {
    Capture capture(str);

    std::set<State *> set1 = {enfa.in->closure()};
    std::set<State *> set2;

    size_t pos = 0;
    for (char c : str)
    {
        if (set1.empty())
            break;
        std::vector<Capture::Event> events;
        for (State * s : set1)
        {
            auto se = s->GetEvents(pos);
            events.insert(events.end(), se.begin(), se.end());
        }
        capture.AddEvents(events);
        for (State * s : set1)
        {
            auto transition = s->GetTransition(c);
            set2.insert(transition.begin(), transition.end());
        }
        set2.swap(set1);
        set2.clear();
        ++pos;
    }
    std::vector<Capture::Event> events;
    for (State * s : set1)
    {
        auto se = s->GetEvents(pos);
        events.insert(events.end(), se.begin(), se.end());
    }
    capture.AddEvents(events);

    std::cout << capture.DebugString() << std::endl;
    capture.Compute();
    std::cout << capture.DebugString() << std::endl;

    return std::find_if(set1.begin(), set1.end(), [](State * s) -> bool {
               return s->IsTerminal();
           }) != set1.end();
}

int main() {
    // std::string regex = "((a)*)";
    // std::string regex = "((a*)|(a)*|(a*)*)";
    std::string regex = "((a(b*)c|d(e*)f)*)";
    // std::string regex = "((a*)|(b*)|(c*))";
    StatePort enfa = FABuilder::Compile(regex);
    // DFA dfa = FABuilder::Compile(enfa);
    // std::cout << dfa.DebugString() << std::endl;

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        std::cout << "ENFA: " << (Match(enfa, line) ? "ok." : "reject.")
                  << std::endl;
        // MatchResult m = dfa.Match(line);
        // std::cout << "DFA:  " << (m.Matched() ? "ok." : "reject.") <<
        // std::endl; if (m.Matched())
        //{
        //    for (size_t i = 0; i <= m.MaxGroupId(); ++i)
        //    {
        //        std::cout << std::ends << "Group " << i << ": \"" <<
        //        m.Group(i)
        //                  << "\"" << std::endl;
        //    }
        //}
    }

    return 0;
}
