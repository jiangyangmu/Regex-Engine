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
    enum ActionType { B, E, BE };
    struct Action {
        size_t group_id;
        ActionType type;
    };
    struct Event {
        size_t group_id;
        ActionType type;
        size_t pos;
    };

public:
    Capture(const std::string & origin)
        : origin_(origin) {
    }

    void AddEvents(const std::map<size_t, Event> & events) {
        for (const auto & kv : events)
        {
            events_[kv.first].emplace_back(kv.second);
        }
    }

    void Compute() {
        if (!captured_.empty())
            return;

        for (const auto & kv : events_)
        {
            const auto & events = kv.second;

            if (events.size() < 2)
                continue;

            // pattern 0      : no capture
            // pattern 1 (a)  : (B,E)*,B | (B,E)+
            // pattern 2 (a)* : (B+,BE*)+
            // pattern 3 (a*) : (BE+,E*)+
            int pattern = EventPattern(events);
            assert(pattern >= 0 && pattern <= 3);
            if (pattern == 0)
                continue;

            size_t group_id = kv.first;
            size_t start, end;
            switch (pattern)
            {
                case 1: // pattern 1 (a)  : (B,E)*,B | (B,E)+
                    for (size_t i = 0; i + 1 < events.size(); i += 2)
                    {
                        start = events[i].pos;
                        end = events[i + 1].pos;
                        captured_[group_id].push_back(
                            origin_.substr(start, end - start));
                    }
                    break;
                case 2: // pattern 2 (a)* : (B+,BE*)+
                    for (size_t i = 0; i < events.size();)
                    {
                        while (i < events.size() && events[i].type == B)
                            ++i;
                        while (i < events.size() && events[i].type == BE)
                        {
                            start = events[i - 1].pos;
                            end = events[i].pos;
                            captured_[group_id].push_back(
                                origin_.substr(start, end - start));
                            ++i;
                        }
                    }
                    break;
                case 3: // pattern 3 (a*) : (BE+,E*)+
                    for (size_t i = 0; i < events.size();)
                    {
                        while (i < events.size() && events[i].type == BE)
                            ++i;
                        start = i;
                        while (i < events.size() && events[i].type == E)
                            ++i;
                        end = i;
                        if (end > start)
                        {
                            start = events[start - 1].pos;
                            end = events[end - 1].pos;
                            captured_[group_id].push_back(
                                origin_.substr(start, end - start));
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    std::string DebugString() const {
        std::string s;
        for (const auto & events : events_)
        {
            s += std::to_string(events.first) + ":";
            for (const auto & e : events.second)
            {
                s += " {";
                assert(e.group_id == events.first);
                switch (e.type)
                {
                    case B:
                        s += "B";
                        break;
                    case E:
                        s += "E";
                        break;
                    case BE:
                        s += "BE";
                        break;
                    default:
                        break;
                }
                s += ",";
                s += std::to_string(e.pos);
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
    int EventPattern(const std::vector<Event> & events) {
        // pattern -1     : invalid capture
        // pattern 0      : no capture
        // pattern 1 (a)  : (B,E)*,B | (B,E)+
        // pattern 2 (a)* : (B+,BE*)+
        // pattern 3 (a*) : (BE+,E*)+
        // pattern 4 (a*)*: BE+           <== TODO: can't compute capture.
        int pattern = -1;
        if (events.size() < 2)
        {
            pattern = 0;
        }
        else
        {
            if (events[0].type == B)
            {
                if (events[1].type == E)
                {
                    pattern = 1;
                    for (size_t i = 2; i < events.size(); ++i)
                    {
                        if (events[i].type != (i % 2 == 0 ? B : E))
                        {
                            pattern = -1;
                            break;
                        }
                    }
                }
                else
                {
                    pattern = 2;
                    bool has_capture = false;
                    for (size_t i = 1; i < events.size(); ++i)
                    {
                        if (events[i].type == BE)
                        {
                            has_capture = true;
                        }
                        if (events[i].type != B && events[i].type != BE)
                        {
                            pattern = -1;
                            break;
                        }
                    }
                    if (!has_capture)
                        pattern = 0;
                }
            }
            else
            {
                pattern = 3;
                bool has_capture = false;
                for (size_t i = 1; i < events.size(); ++i)
                {
                    if (events[i].type == E)
                    {
                        has_capture = true;
                    }
                    else if (events[i].type == B)
                    {
                        pattern = -1;
                        break;
                    }
                }
            }
        }
        return pattern;
    }

    std::map<size_t, std::vector<Event>> events_;
    std::map<size_t, std::vector<std::string>> captured_;
    const std::string & origin_;
};

class Char {
public:
    Char(char c)
        : encode_(c - 'a' + 1) {
        assert(c >= 'a' && c <= 'z');
    }
    operator uint8_t() const {
        return encode_;
    }
    char ToChar() const {
        assert(encode_ != 0);
        return encode_ + 'a';
    }
    static size_t CharTypeCount() {
        return 27;
    }
    static const Char epsilon;

private:
    Char()
        : encode_(0) {
    }

    uint8_t encode_;
};
const Char Char::epsilon;

class State {
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
        assert(actions_.find(action.group_id) == actions_.end());
        actions_[action.group_id] = action.type;
    }
    const std::map<size_t, Capture::ActionType> & GetActions() const {
        return actions_;
    }

private:
    int id_;
    bool is_terminal_;
    std::map<Char, std::set<State *>> transition_;
    std::set<State *> eclosure_;
    std::map<size_t, Capture::ActionType> actions_;
};

struct StatePort {
    State * in;
    State * out;
};

class MatchResult {
public:
    MatchResult(const std::string & origin)
        : capture_(origin)
        , matched_(false) {
    }
    void SetMatched() {
        matched_ = true;
    }
    bool Matched() const {
        return matched_;
    }
    Capture & capture() {
        return capture_;
    }

private:
    Capture capture_;
    bool matched_;
};

std::map<size_t, Capture::ActionType> MergeStateActions(
    const std::set<State *> & states) {
    std::map<size_t, Capture::ActionType> actions;
    for (State * s : states)
    {
        for (auto kv : s->GetActions())
        {
            size_t group_id = kv.first;
            Capture::ActionType type = kv.second;
            if (actions.find(group_id) == actions.end())
            {
                actions[group_id] = type;
            }
            else
            {
                assert(
                    (actions[group_id] == Capture::B && type == Capture::E) ||
                    (actions[group_id] == Capture::E && type == Capture::B));
                actions[group_id] = Capture::BE;
            }
        }
    }
    return actions;
}

std::map<size_t, Capture::Event> MakeStateEvents(
    const std::map<size_t, Capture::ActionType> & actions, size_t pos) {
    std::map<size_t, Capture::Event> events;
    for (auto kv : actions)
    {
        size_t group_id = kv.first;
        Capture::ActionType type = kv.second;
        events[group_id] = {group_id, type, pos};
    }
    return events;
}

std::map<size_t, Capture::Event> MakeStateEvents(
    const std::set<State *> & states, size_t pos) {
    std::map<size_t, Capture::Event> events;
    for (State * s : states)
    {
        for (auto kv : s->GetActions())
        {
            size_t group_id = kv.first;
            Capture::ActionType type = kv.second;
            if (events.find(group_id) == events.end())
            {
                events[group_id] = {group_id, type, pos};
            }
            else
            {
                assert((events[group_id].type == Capture::B &&
                        type == Capture::E) ||
                       (events[group_id].type == Capture::E &&
                        type == Capture::B));
                events[group_id].type = Capture::BE;
            }
        }
    }
    return events;
}

class DFA {
    friend class FABuilder;

public:
    MatchResult Match(const std::string & str) const {
        MatchResult result(str);

        int s = 0;
        size_t pos = 0;
        for (char c : str)
        {
            result.capture().AddEvents(MakeStateEvents(actions[s], pos));
            s = GetTransition(s, c);
            if (s == -1)
                return MatchResult(str);
            ++pos;
        }
        result.capture().AddEvents(MakeStateEvents(actions[s], pos));
        if (IsTerminal(s))
        {
            result.capture().Compute();
            result.SetMatched();
        }
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
    int AddState(const std::set<State *> & states) {
        transition.insert(transition.end(), Char::CharTypeCount(), -1);
        is_terminal.push_back(false);
        actions.push_back(MergeStateActions(states));
        return (int)(is_terminal.size() - 1);
    }
    void SetTransition(int from, Char c, int to) {
        assert(from >= 0 &&
               (from + 1) * Char::CharTypeCount() <= (int)transition.size() &&
               to >= 0 &&
               (to + 1) * Char::CharTypeCount() <= (int)transition.size());
        transition[from * Char::CharTypeCount() + c] = to;
    }
    int GetTransition(int state, Char c) const {
        assert(state >= 0 &&
               (state + 1) * Char::CharTypeCount() <= (int)transition.size());
        return transition[state * Char::CharTypeCount() + c];
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
    std::vector<std::map<size_t, Capture::ActionType>> actions;
};

class FABuilder {
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
        a->AddAction({group_id, Capture::B});
        b->AddAction({group_id, Capture::E});
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
        sidmap[H(q.front())] = dfa.AddState(q.front());
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
                    sidmap[H(s2)] = dfa.AddState(s2);
                    q.push_back(s2);
                }
                int to = sidmap[H(s2)];
                dfa.SetTransition(from, c, to);
            }
        }

        return dfa;
    }

private:
    std::vector<StatePort> ports_;
    std::vector<size_t> pos_;
    std::vector<size_t> group_ids_;
    size_t group_idgen_;
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
        capture.AddEvents(MakeStateEvents(set1, pos));
        for (State * s : set1)
        {
            auto transition = s->GetTransition(c);
            set2.insert(transition.begin(), transition.end());
        }
        set2.swap(set1);
        set2.clear();
        ++pos;
    }
    capture.AddEvents(MakeStateEvents(set1, pos));

    capture.Compute();
    std::cout << capture.DebugString() << std::endl;

    return std::find_if(set1.begin(), set1.end(), [](State * s) -> bool {
               return s->IsTerminal();
           }) != set1.end();
}

int main() {
    // std::string regex = "((a)*)";
    // std::string regex = "((a*)|(a)*|(a*)*)";
    // std::string regex = "((a(b*)c)*)";
    std::string regex = "((a(b)*c|d(e*)f)*)";
    // std::string regex = "((a*)|(b*)|(c*))";
    StatePort enfa = FABuilder::Compile(regex);
    DFA dfa = FABuilder::Compile(enfa);
    // std::cout << dfa.DebugString() << std::endl;

    std::cout << "pattern: " << regex << std::endl;
    for (std::string line; std::getline(std::cin, line);)
    {
        std::cout << "ENFA: " << (Match(enfa, line) ? "ok." : "reject.")
                  << std::endl;
        MatchResult m = dfa.Match(line);
        if (m.Matched())
            std::cout << m.capture().DebugString() << std::endl;
        std::cout << "DFA:  " << (m.Matched() ? "ok." : "reject.") << std::endl;
    }

    return 0;
}
