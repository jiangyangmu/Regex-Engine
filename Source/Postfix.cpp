#include "Postfix.h"

#include "stdafx.h"
#include "util.h"

#include "CharMatcher.h"

RString PostfixNode::DebugString() const {
    RString s;
    switch (type)
    {
        case NULL_INPUT:
            s += L"INPUT()";
            break;
        case CHAR_INPUT:
            s += L"INPUT(";
            s += chr;
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
                    s += L"CAPTURE(" + std::to_wstring(group.capture_id) + L")";
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
    explicit PostfixParser(RView regex)
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
        if (regex_.Match('*'))
        {
            Repeat r = {repeat_id_gen_++, 0, 0, false};
            nl_.emplace_back(r);
        }
        else if (regex_.Match('{'))
        {
            Repeat r = {repeat_id_gen_++, 0, 0, false};

            regex_.MatchUInt32(&r.min);
            assert(regex_.Match(','));

            if (regex_.MatchUInt32(&r.max))
            {
                r.has_max = true;
                assert(r.max >= r.min);
            }

            assert(regex_.Match('}'));

            nl_.emplace_back(r);
        }
    }
    bool item() {
        if (regex_.Match('\\'))
        {
            size_t backref_id = 10;
            assert(regex_.MatchUInt32(&backref_id));
            assert(backref_id <= 9);
            BackReference b = { (int)backref_id };
            nl_.emplace_back(b);
            repeat();
            return true;
        }
        else if (regex_.TryMatch('('))
        {
            group();
            repeat();
            return true;
        }
        else
        {
            // TODO: check not special char
            if (!regex_.TryMatchAny(L"|)"))
            {
                nl_.emplace_back(regex_.GetChar());
                repeat();
                return true;
            }
            else
                return false;
        }
    }
    void concat() {
        assert(item());
        while (item())
            nl_.emplace_back(PostfixNode::CONCAT);
    }
    void alter() {
        if (regex_.TryMatchAny(L"|)"))
        {
            nl_.emplace_back(PostfixNode::NULL_INPUT);
            return;
        }

        concat();
        while (regex_.Match('|'))
        {
            if (regex_.TryMatchAny(L"|)"))
            {
                nl_.emplace_back(PostfixNode::NULL_INPUT);
                continue;
            }
            concat();
            nl_.emplace_back(PostfixNode::ALTER);
        }
    }
    void group() {
        assert(regex_.Match('('));

        Group g;

        g.type = Group::CAPTURE;
        if (regex_.MatchTwo('?',':'))
            g.type = Group::NON_CAPTURE;
        else if (regex_.MatchTwo('?', '='))
            g.type = Group::LOOK_AHEAD, g.lookaround_id = lookaround_id_gen_++;
        else if (regex_.MatchTwo('?', '<'))
            g.type = Group::LOOK_BEHIND, g.lookaround_id = lookaround_id_gen_++;
        else if (regex_.MatchTwo('?', '>'))
            g.type = Group::ATOMIC, g.atomic_id = atomic_id_gen_++;
        else
            g.capture_id = capture_id_gen_++;

        alter();

        nl_.emplace_back(g);

        assert(regex_.Match(')'));
    }

    int capture_id_gen_;
    int lookaround_id_gen_;
    int atomic_id_gen_;
    int repeat_id_gen_;
    LexMatcher regex_;
    std::vector<PostfixNode> nl_;
};

std::vector<PostfixNode> ParseToPostfix(RString regex) {
    return PostfixParser(regex.data()).Parse();
}

struct PostfixTreeNode {
    const PostfixNode * node;
    PostfixTreeNode *left, *right;

    RString DebugString() const {
        RString str = L"(" + node->DebugString();
        if (left)
            str += L" " + left->DebugString();
        if (right)
            str += L" " + right->DebugString();
        str += L")";
        return str;
    }
    RString PrettyDebugString() const {
        RString str = DebugString();

        RString pretty_str;
        RString indent;
        for (wchar_t c : str)
        {
            if (c == L'(')
            {
                pretty_str += L'\n';
                pretty_str += indent;
                indent += L"  ";
            }
            else if (c == L')')
            {
                indent.pop_back();
                indent.pop_back();
            }
            pretty_str += c;
        }
        pretty_str += L'\n';

        return pretty_str;
    }
};

std::vector<PostfixNode> FlipPostfix(const std::vector<PostfixNode> & nl) {
    std::vector<std::unique_ptr<PostfixTreeNode>> tree_alloc;

    // build tree and mark flip nodes.
    std::vector<PostfixTreeNode *> tree;
    std::vector<PostfixTreeNode *> flip;
    for (const PostfixNode & node : nl)
    {
        switch (node.type)
        {
            case PostfixNode::CHAR_INPUT:
            case PostfixNode::BACKREF_INPUT:
                tree_alloc.emplace_back(
                    new PostfixTreeNode{&node, nullptr, nullptr});
                tree.push_back(tree_alloc.back().get());
                break;
            case PostfixNode::REPEAT:
            case PostfixNode::GROUP:
                tree_alloc.emplace_back(
                    new PostfixTreeNode{&node, tree.back(), nullptr});
                tree.pop_back();
                tree.push_back(tree_alloc.back().get());
                if (node.type == PostfixNode::GROUP &&
                    node.group.type == Group::LOOK_BEHIND)
                {
                    flip.push_back(tree.back());
                }
                break;
            case PostfixNode::CONCAT:
            case PostfixNode::ALTER:
                tree_alloc.emplace_back(new PostfixTreeNode{
                    &node, tree[tree.size() - 2], tree[tree.size() - 1]});
                tree.pop_back();
                tree.pop_back();
                tree.push_back(tree_alloc.back().get());
                break;
            default:
                assert(false);
                break;
        }
    }

    assert(tree.size() == 1);
    // std::wcout << L"Before:" << tree.front()->PrettyDebugString() <<
    // std::endl;

    // do flips.
    for (PostfixTreeNode * root : flip)
    {
        assert(root->right == nullptr);
        std::vector<PostfixTreeNode *> stack = {root->left};
        while (!stack.empty())
        {
            PostfixTreeNode * n = stack.back();
            stack.pop_back();
            assert(n);
            switch (n->node->type)
            {
                case PostfixNode::CHAR_INPUT:
                case PostfixNode::BACKREF_INPUT:
                    assert(n->left == nullptr);
                    assert(n->right == nullptr);
                    break;
                case PostfixNode::REPEAT:
                    stack.push_back(n->left);
                    assert(n->right == nullptr);
                    break;
                case PostfixNode::CONCAT:
                    std::swap(n->left, n->right);
                    stack.push_back(n->left);
                    stack.push_back(n->right);
                    break;
                case PostfixNode::ALTER:
                    stack.push_back(n->left);
                    stack.push_back(n->right);
                    break;
                case PostfixNode::GROUP:
                    if (n->node->group.type != Group::LOOK_AHEAD &&
                        n->node->group.type != Group::LOOK_BEHIND)
                        stack.push_back(n->left);
                    assert(n->right == nullptr);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }

    // std::wcout << L"After:" << tree.front()->PrettyDebugString() <<
    // std::endl;

    std::vector<PostfixNode> flip_nl;

    std::vector<PostfixTreeNode *> stack = {tree.front()};
    std::set<PostfixTreeNode *> visited;
    while (!stack.empty())
    {
        PostfixTreeNode * n = stack.back();
        if (visited.find(n) == visited.end())
        {
            if (n->right)
                stack.push_back(n->right);
            if (n->left)
                stack.push_back(n->left);
            visited.insert(n);
        }
        else
        {
            stack.pop_back();
            flip_nl.push_back(*n->node);
        }
    }

    return flip_nl;
}
