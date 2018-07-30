#include "Postfix.h"

#include "stdafx.h"
#include "util.h"

CharArray PostfixNode::DebugString() const
{
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

class PostfixParser {
public:
    explicit PostfixParser(const Char::Type * regex)
        : capture_id_gen_(0)
        , lookaround_id_gen_(0)
        , atomic_id_gen_(0)
        , repeat_id_gen_(0)
        , regex_(regex) {
    }
    std::vector<PostfixNode> Parse() {
        group();
        return nl_;
    }

private:
    // TODO: check regex_ boundary.
    void repeat() {
        if (*regex_ == '*')
        {
            ++regex_;
            Repeat r = { repeat_id_gen_++, 0, 0, false };
            nl_.emplace_back(r);
        }
        else if (*regex_ == '{')
        {
            ++regex_;
            Repeat r = { repeat_id_gen_++, 0, 0, false };

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
            BackReference b = { *regex_++ - '0' };
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
            Char c = { *regex_++ };
            nl_.emplace_back(c);
            repeat();
            return true;
        }
        return false;
    }
    void concat() {
        assert(item());
        while (item())
            nl_.emplace_back(PostfixNode::CONCAT);
    }
    void alter() {
        if (*regex_ == '|' || *regex_ == ')')
        {
            nl_.emplace_back(PostfixNode::NULL_INPUT);
            return;
        }

        concat();
        while (*regex_ == '|')
        {
            ++regex_;
            if (*regex_ == '|' || *regex_ == ')')
            {
                nl_.emplace_back(PostfixNode::NULL_INPUT);
                continue;
            }
            concat();
            nl_.emplace_back(PostfixNode::ALTER);
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
    std::vector<PostfixNode> nl_;
};

std::vector<PostfixNode> ParseToPostfix(CharArray regex)
{
    return PostfixParser(regex.data()).Parse();
}
