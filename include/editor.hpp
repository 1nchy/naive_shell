#ifndef _ASP_EDITOR_HPP_
#define _ASP_EDITOR_HPP_

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <string>

#include <iostream>

#include "virtual_editor.hpp"
#include "signal_stack/signal_stack.hpp"

namespace asp {
class editor;
extern editor default_editor;

class editor : public shell_editor {
public:
    // editor(std::istream& _is = std::cin, std::ostream& _os = std::cout, const std::string& _prompt = "> ");
    editor(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO, const std::string& _prompt = "> ");
    editor(const editor&) = delete;
    editor& operator=(const editor&) = delete;
    ~editor();

    bool eof() const { return _end_of_file; }
    int in() const override { return _in; }
    int out() const override { return _out; }
    int err() const override { return _err; }
    void show_information(const std::string& _s) override { write(_out, _s.data(), _s.size()); }
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
    ssize_t _M_error(char _c) { return write(_err, &_c, sizeof(char)); }
    ssize_t _M_error(const std::string& _s) { return write(_err, _s.data(), _s.size() * sizeof(char)); }

private:
    std::vector<std::string> _history;
    typename decltype(_history)::const_iterator _history_pointer;
    std::vector<char> _front;
    std::vector<char> _back;
    std::string _command;
    const std::string _prompt = "> ";
    bool _end_of_file = false;

    int _in; int _out; int _err;

    signal_stack _local_ss;

    // std::istream& _is;
    // std::ostream& _os;
};
};

#endif // _ASP_EDITOR_HPP_