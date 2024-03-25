#ifndef _ASP_TRIE_TREE_HPP_
#define _ASP_TRIE_TREE_HPP_

#include <string>
#include <vector>

#include <unordered_map>

namespace asp {
class trie_tree;
// struct trie_tree_node;

struct trie_tree_node {
    typedef trie_tree_node self;
    const char _c;
    bool _end_of_word = false;
    std::unordered_map<char, self*> _children;
    trie_tree_node(char _c, bool _eow = false) : _c(_c), _end_of_word(_eow) {}
    ~trie_tree_node() {
        for (auto& _i : _children) {
            delete _i.second;
        }
        _children.clear();
    }
    bool contains(char _c) const {
        return _children.contains(_c);
    }
    void add(char _c) {
        _children.emplace(_c, new self(_c));
    }
    bool del(char _c) {
        if (!contains(_c)) return false;
        delete _children.at(_c);
        _children.erase(_c);
        return true;
    }
};

class trie_tree {
    trie_tree_node _root;
    size_t _word_cnt = 0;
    size_t _max_depth = 0;
public:
    trie_tree() : _root(0) {}
    trie_tree(std::initializer_list<const char* const>);
    trie_tree(const trie_tree&) = delete;
    trie_tree& operator=(const trie_tree&) = delete;
    ~trie_tree() = default;
    void add(const std::string& _s);
    bool del(const std::string& _s);
    void clear();
    /**
     * @brief whether the tree contains %_s, return false if %_s.empty()
    */
    bool query(const std::string& _s) const;
    /**
     * @brief list all completions prefixed with %_s, return all words if %_s.empty()
    */
    std::vector<std::string> tab(const std::string& _s) const;
    /**
     * @brief common prefix of all completions prefixed with %_s, all words prefixed with ""
    */
    std::string next(const std::string& _s) const;
    /**
     * @brief longest string match
    */
    size_t longest_match(std::string::const_iterator _begin, std::string::const_iterator _end) const;
private:
    trie_tree_node* locate(const std::string& _s);
    const trie_tree_node* locate(const std::string& _s) const;
    std::vector<trie_tree_node*> trace(const std::string& _s);
};
};

#endif // _ASP_TRIE_TREE_HPP_