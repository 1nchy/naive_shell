#ifndef _ASP_EDITOR_HPP_
#define _ASP_EDITOR_HPP_

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <string>

#include <iostream>

#include "virtual_editor.hpp"
#include "signal_stack.hpp"

namespace asp {
class editor;
extern editor default_editor;

class editor : public shell_editor {
public:
    // editor(std::istream& _is = std::cin, std::ostream& _os = std::cout, const std::string& _prompt = "> ");
    editor(int _in = fileno(stdin), int _out = fileno(stdout), const std::string& _prompt = "> ");
    editor(const editor&) = delete;
    editor& operator=(const editor&) = delete;
    ~editor();

    bool eof() const { return _end_of_file; }
    // std::istream& in() override { return _is; }
    int in() const override { return _in; }
    // std::ostream& out() override { return _os; }
    int out() const override { return _out; }
    // void show_information() override { _os << "[1nchy]";}
    void show_information(const std::string& _s) override { write(_out, _s.data(), _s.size()); }
    // void show_prompt() override { _os << _prompt; }
    void show_prompt() override { write(_out, _prompt.c_str(), _prompt.size()); }
    const std::string& line() override;

private:
    bool wait();
    void back(size_t);
    void del(size_t);
    void cursor_back(size_t); // move cursor back
    void viewer_back(size_t);
    void fill_blank(size_t);
    void write_front();
    void write_back(size_t _i = 0);
    void tab();
    inline bool empty() const { return _front.empty() && _back.empty(); }
    inline void clear() { _front.clear(); _back.clear(); _command.clear(); }
    void build_command();

    void load_history();

    void _M_cooked();
    void _M_raw();

    // void erase_combine_key();
    ssize_t _M_read(char& _c) { return read(_in, &_c, sizeof(char)); }
    ssize_t _M_write(char _c) { return write(_out, &_c, sizeof(char)); }
    ssize_t _M_write(const std::string& _s) { return write(_out, _s.data(), _s.size() * sizeof(char)); }

private:
    std::vector<std::string> _history;
    typename decltype(_history)::const_iterator _history_pointer;
    std::vector<char> _front;
    std::vector<char> _back;
    std::string _command;
    const std::string _prompt = "> ";
    bool _end_of_file = false;

    int _in; int _out;

    signal_stack _local_ss;

    // std::istream& _is;
    // std::ostream& _os;
};
};

#endif // _ASP_EDITOR_HPP_