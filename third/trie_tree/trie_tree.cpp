#include "trie_tree.hpp"

#include <cassert>

#include <functional>

namespace asp {
auto trie_tree::locate(const std::string& _s)
-> trie_tree_node* {
    trie_tree_node* _p = &_root;
    for (const auto _c : _s) {
        if (_p->contains(_c)) {
            _p = _p->_children.at(_c);
        }
        else {
            return &_root;
        }
    }
    return _p;
}
auto trie_tree::locate(const std::string& _s) const
-> const trie_tree_node* {
    const trie_tree_node* _p = &_root;
    for (const auto _c : _s) {
        if (_p->contains(_c)) {
            _p = _p->_children.at(_c);
        }
        else {
            return &_root;
        }
    }
    return _p;
}
auto trie_tree::trace(const std::string& _s)
-> std::vector<trie_tree_node*> {
    std::vector<trie_tree_node*> _path;
    _path.reserve(_max_depth);
    trie_tree_node* _p = &_root;
    for (const auto _c : _s) {
        if (_p->contains(_c)) {
            _p = _p->_children.at(_c);
            _path.emplace_back(_p);
        }
        else {
            return {};
        }
    }
    return _path;
}


trie_tree::trie_tree(std::initializer_list<const char* const> _il)
: _root(0) {
    for (const auto& _s : _il) {
        add(std::string(_s));
    }
}

auto trie_tree::add(const std::string& _s)
-> void {
    trie_tree_node* _p = &_root;
    size_t _depth = 0;
    for (const auto _c : _s) {
        if (_p->contains(_c)) {
            _p = _p->_children.at(_c);
        }
        else {
            _p->add(_c);
            ++_depth;
            _p = _p->_children.at(_c);
        }
    }
    _p->_end_of_word = true;
    ++_word_cnt;
    _max_depth = std::max(_max_depth, _depth);
}
auto trie_tree::del(const std::string& _s)
-> bool {
    std::vector<trie_tree_node*> _path = trace(_s);
    if (_path.empty()) return false;
    trie_tree_node* _p = _path.back();
    assert(_p->_end_of_word); _p->_end_of_word = false;
    char _to_del_key = 0;
    for (auto _i = _path.rbegin(); _i != _path.rend(); ++_i) {
        _p = *_i;
        _p->del(_to_del_key);
        if (_p->_children.size() >= 1) break;
        _to_del_key = _p->_c;
    }
    return true;
}
auto trie_tree::query(const std::string& _s) const
-> bool {
    const trie_tree_node* _p = locate(_s);
    return _p == &_root;
}
auto trie_tree::tab(const std::string& _s) const
-> std::vector<std::string> {
    const trie_tree_node* _p = locate(_s);
    if (_p == &_root) return {};
    std::vector<std::string> _completion;
    std::string _path; _path.reserve(_max_depth);
    std::function<void(const trie_tree_node* _p)> dfs
    = [&](const trie_tree_node* _p) {
        if (_p->_end_of_word) {
            _completion.emplace_back(_path);
        }
        for (const auto& _i : _p->_children) {
            _path.push_back(_i.first);
            const trie_tree_node* _k = _i.second;
            dfs(_k);
            _path.pop_back();
        }
    };
    dfs(_p);
    return _completion;
}
auto trie_tree::next(const std::string& _s) const
-> std::string {
    // const trie_tree_node* _p = locate(_s);
    const trie_tree_node* _p = &_root;
    for (const auto _c : _s) {
        if (_p->contains(_c)) {
            _p = _p->_children.at(_c);
        }
        else {
            _p = &_root; break;
        }
    }
    if (_p == &_root) return {};
    std::string _completion;
    while (_p->_children.size() == 1 && !_p->_end_of_word) {
        _p = _p->_children.cbegin()->second;
        _completion.push_back(_p->_c);
    }
    return _completion;
}
auto trie_tree::longest_match(std::string::const_iterator _begin, std::string::const_iterator _end) const
-> size_t {
    std::string::const_iterator _i = _begin;
    const trie_tree_node* _p = &_root;
    size_t _max_match = 0;
    while (_i != _end) {
        if (_p->contains(*_i)) {
            _p = _p->_children.at(*_i); ++_i;
            if (_p->_end_of_word) _max_match = _i - _begin;
        }
        else {
            break;
        }
    }
    return _max_match;
}
};