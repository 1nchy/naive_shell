#include "frontend.hpp"
#include "backend.hpp"

#include <termios.h>

#include "signal_stack/signal_stack.hpp"

namespace asp {

static signal_stack _signal_stack;

shell_frontend::shell_frontend(int _in, int _out, int _err) {
    load_history();
    _backend = &shell_backend::singleton(_in, _out, _err);
}
shell_frontend& shell_frontend::singleton(int _in, int _out, int _err) {
    static shell_frontend _instance(_in, _out, _err);
    return _instance;
}
shell_frontend::~shell_frontend() {}
void shell_frontend::show_information(const std::string& _s) {
    _M_write(_s);
}
void shell_frontend::show_prompt() {
    _M_write(_prompt);
}
void shell_frontend::run() {
    while (true) {
        if (!wait()) break;
        if (!_backend->compile()) continue;
        _backend->execute();
    }
}
bool shell_frontend::wait() {
    show_information(_backend->build_information());
    while (read_line()) {
        build_line();
        const auto _r = _backend->parse(_command_line);
        if (_r == -1) return false;
        else if (_r == 0) return true;
    }
    return false;
}
bool shell_frontend::read_line() {
    clear(); char _c;
    show_prompt();
    _end_of_file = false;
    _M_start_handler();
    while (true) {
    const auto _r = _M_read(_c);
    if (_r == -1) {
        if (errno == EINTR) continue;
        else {
            _M_end_handler();
            return false;
        }
    }
    else if (_r == 0) {
        _end_of_file = true;
        _M_end_handler();
        return false;
    }
    // input normally
    if (_c == '\r' || _c == '\n') {
        rewrite_back();
        if (empty()) { // todo
            continue;
        }
        build_line();
        _backend->append_history(_command_line);
        _M_end_handler();
        return true;
    }
    else if (_c == '\04' || _c == '\03') { // ctrl+c or ctrl+d
        rewrite_back();
        _end_of_file = true;
        _M_end_handler();
        return false;
    }
    else if (_c == '\033') { // _c == '^['
        char _subc;
        _M_read(_subc);
        _M_read(_subc);
        if (_subc == 'A') { // up arrow ^[[A
            cursor_move_back(_front.size());
            fill_blank(0);
            cursor_move_back(_front.size() + _back.size());
            const auto& _prev_cmd = _backend->prev_history();
            if (_prev_cmd.empty()) {
                clear();
            }
            else {
                _M_write(_prev_cmd);
                clear();
                _front.assign(_prev_cmd.cbegin(), _prev_cmd.cend());
            }
        }
        else if (_subc == 'B') { // down arrow ^[[B
            cursor_move_back(_front.size());
            fill_blank(0);
            cursor_move_back(_front.size() + _back.size());
            const auto& _next_cmd = _backend->next_history();
            if (_next_cmd.empty()) {
                clear();
            }
            else {
                _M_write(_next_cmd);
                clear();
                _front.assign(_next_cmd.cbegin(), _next_cmd.cend());
            }
        }
        else if (_subc == 'D') { // left arrow ^[[D
            if (!_front.empty()) {
                _back.push_back(_front.back());
                _front.pop_back();
                cursor_move_back(1);
            }
        }
        else if (_subc == 'C') { // right arrow ^[[C
            if (!_back.empty()) {
                rewrite_back();
                _front.push_back(_back.back());
                _back.pop_back();
                cursor_move_back(_back.size());
            }
        }
        else if (_subc == 'H') { // home key ^[[H
            rewrite_back();
            _back.insert(_back.end(), _front.crbegin(), _front.crend());
            _front.clear();
            cursor_move_back(_back.size());
        }
        else if (_subc == 'F') { // end key ^[[F
            rewrite_back();
            _front.insert(_front.end(), _back.crbegin(), _back.crend());
            _back.clear();
        }
        else if (_subc == '3') { // del key ^[[3~
            _M_read(_subc); // get '~'
            if (!_back.empty()) {
                // viewer_back(5);
                _back.pop_back();
                rewrite_back();
                fill_blank(1);
                cursor_move_back(_back.size() + 1);
            }
        }
    }
    else if (_c == 127) { // backspace
        if (!_front.empty()) {
            text_backspace(1);
            _front.pop_back();
            rewrite_back();
            fill_blank(1);
            cursor_move_back(_back.size() + 1);
        }
    }
    else if (_c == '\t') {

    }
    else {
        _front.push_back(_c);
        _M_write(_c);
        rewrite_back();
        cursor_move_back(_back.size());
    }
    }
    _M_end_handler();
    return true;
}


void shell_frontend::load_history() {}


void shell_frontend::cursor_move_back(size_t _n) {
    for (size_t _i = 0; _i < _n; ++_i) _M_write('\b');
}
void shell_frontend::fill_blank(size_t _n) {
    _n = (_n ? _n : _front.size() + _back.size());
    for (size_t _i = 0; _i < _n; ++_i) _M_write(' ');
}
void shell_frontend::text_backspace(size_t _n) {
    cursor_move_back(_n);
    fill_blank(_n);
    cursor_move_back(_n);
}
void shell_frontend::rewrite_front() {
    std::for_each(_front.cbegin(), _front.cend(), [this](char _c) {
        this->_M_write(_c);
    });
}
void shell_frontend::rewrite_back() {
    std::for_each(_back.crbegin(), _back.crend(), [this](char _c) {
        this->_M_write(_c);
    });
}
void shell_frontend::build_line() {
    _command_line.clear();
    _command_line.reserve(_front.size() + _back.size());
    _command_line.append(_front.cbegin(), _front.cend());
    _command_line.append(_back.crbegin(), _back.crend());
}


void shell_frontend::_M_cooked() {
    _signal_stack.build(SIGCHLD, nullptr);
    system("stty cooked");
    _signal_stack.restore(SIGCHLD);
}
void shell_frontend::_M_raw() {
    _signal_stack.build(SIGCHLD, nullptr);
    system("stty raw");
    _signal_stack.restore(SIGCHLD);
}
void shell_frontend::_M_start_handler() {
    _M_raw();
    tcflush(_in, TCIFLUSH);
    tcgetattr(_in, &_default_in_setting);
    _shell_in_setting = _default_in_setting;
    _shell_in_setting.c_lflag &= ~ECHO;
    tcsetattr(_in, TCSANOW, &_shell_in_setting);
}
void shell_frontend::_M_end_handler() {
    tcflush(_in, TCIFLUSH);
    tcsetattr(_in, TCSANOW, &_default_in_setting);
    _M_cooked();
    _M_write('\n');
}

ssize_t shell_frontend::_M_read(char& _c) {
    return read(_in, &_c, sizeof(char));
}
ssize_t shell_frontend::_M_write(char _c) {
    return write(_out, &_c, sizeof(char));
}
ssize_t shell_frontend::_M_write(const std::string& _s) {
    return write(_out, _s.data(), _s.size() * sizeof(char));
}
ssize_t shell_frontend::_M_error(char _c) {
    return write(_err, &_c, sizeof(char));
}
ssize_t shell_frontend::_M_error(const std::string& _s) {
    return write(_err, _s.data(), _s.size() * sizeof(char));
}

};