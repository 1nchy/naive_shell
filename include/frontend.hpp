#ifndef _ASP_FRONTEND_HPP_
#define _ASP_FRONTEND_HPP_

#include "virtual_frontend.hpp"

namespace asp {

class shell_frontend;

class shell_frontend : public frontend_interface {
private:
    shell_frontend(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
public:
    static shell_frontend& singleton(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
    virtual ~shell_frontend();
    bool eof() const { return _end_of_file; }
    void show_information(const std::string& _s) override;
    void show_prompt() override;
    void run() override;
private:
    bool wait() override;
    bool read_line();
private: // history
    void load_history();
private: // view
    void cursor_move_back(size_t); // move cursor back
    void fill_blank(size_t);
    void text_backspace(size_t);
    void rewrite_front();
    void rewrite_back();
    bool empty() const { return _front.empty() && _back.empty(); }
    void clear() { _front.clear(); _back.clear(); _command_line.clear(); }
    void build_line();
private: // termios
    void _M_cooked();
    void _M_raw();
    void _M_start_handler();
    void _M_end_handler();
public: // ansi color mode
    enum ansi_color {
        default_color = 0,
        black, red, green, yellow, blue, magenta, cyan, white,
        color_count,
    };
    static const char* color_mode[];
private: // I/O
    ssize_t _M_read(char& _c);
    ssize_t _M_write(char _c);
    ssize_t _M_write(const std::string& _s);
    ssize_t _M_error(char _c);
    ssize_t _M_error(const std::string& _s);
private:
    std::vector<char> _front;
    std::vector<char> _back;
    const std::string _prompt = "> ";
    bool _end_of_file = false;
    termios _shell_in_setting, _default_in_setting;
};

};

#endif // _ASP_FRONTEND_HPP_