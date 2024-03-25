#include "frontend.hpp"
#include "backend.hpp"

#include <termios.h>

#include "signal_stack/signal_stack.hpp"

namespace asp {

static signal_stack _signal_stack;

const char* shell_frontend::color_mode[] = {
    "\x1b[39;49m",
    "\x1b[30m", "\x1b[31m", "\x1b[32m", "\x1b[33m",
    "\x1b[34m", "\x1b[35m", "\x1b[36m", "\x1b[37m",
};
static_assert(sizeof(shell_frontend::color_mode) / sizeof(shell_frontend::color_mode[0]) == shell_frontend::ansi_color::color_count);

shell_frontend::shell_frontend(int _in, int _out, int _err) {
    _backend = &shell_backend::singleton(_in, _out, _err);
}
shell_frontend& shell_frontend::singleton(int _in, int _out, int _err) {
    static shell_frontend _instance(_in, _out, _err);
    return _instance;
}
shell_frontend::~shell_frontend() {}
void shell_frontend::show_information(const std::string& _s) {
    _M_write(color_mode[green] + _s + color_mode[0]);
}
void shell_frontend::show_prompt() {
    _M_write(color_mode[blue] + _prompt + color_mode[0]);
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
    _M_term_start_handler();
    while (!_end_of_file) {
    const auto _r = _M_read(_c);
    if (_r == -1) {
        if (errno == EINTR) continue;
        else {
            _M_term_end_handler();
            return false;
        }
    }
    else if (_r == 0) {
        _end_of_file = true;
        _M_term_end_handler();
        return false;
    }
    // input normally
    if (_c == '\r' || _c == '\n') {
        (this->*_char_handler_map.at(_c))(_c);
        return true;
    }
    else if (_c == '\04' || _c == '\03') { // ctrl+c or ctrl+d
        (this->*_char_handler_map.at(_c))(_c);
        return false;
    }
    else if (_char_handler_map.contains(_c)) {
        (this->*_char_handler_map.at(_c))(_c);
    }
    else {
        (this->*_char_handler_map.at(0))(_c);
    }
    }
    _M_term_end_handler();
    return true;
}


void shell_frontend::enter_handler(char) {
    rewrite_back();
    if (empty()) {} // todo
    build_line();
    _backend->append_history(_command_line);
    _M_term_end_handler();
}
void shell_frontend::end_handler(char) {
    rewrite_back();
    _end_of_file = true;
    _M_term_end_handler();
}
void shell_frontend::escape_handler(char) {
    char _subc;
    _M_read(_subc);
    _M_read(_subc);
    if (_subc == 'A') { // up arrow ^[[A
        cursor_move_back(_front.size());
        fill_blank(_front.size() + _back.size());
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
        fill_blank(_front.size() + _back.size());
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
void shell_frontend::backspace_handler(char) {
    if (!_front.empty()) {
        text_backspace(1);
        _front.pop_back();
        rewrite_back();
        fill_blank(1);
        cursor_move_back(_back.size() + 1);
    }
}
void shell_frontend::tab_handler(char) {
    const std::string _line_2bc(_front.cbegin(), _front.cend());
    do {
        if (!has_tab_next()) {
            const std::string _tab_next = _backend->build_tab_next(_line_2bc);
            if (!_tab_next.empty()) {
                // insert %_tab_next into _front
                _M_write(_tab_next);
                _front.insert(_front.end(), _tab_next.cbegin(), _tab_next.cend());
                rewrite_back();
                cursor_move_back(_back.size());
                _tab_next_signature = front_signature();
                break;
            }
            _tab_next_signature = front_signature();
        }
        if (!has_tab_list()) {
            _tab_list = _backend->build_tab_list(_line_2bc);
            _tab_index = _tab_list.size() - 1;
            _tab_list_signature = front_signature();
            // _tab_list ready
            if (_tab_list.empty()) break;
            switch_tab_list();
        }
        else { // unused branch
            _tab_list_signature = front_signature();
            if (_tab_list.empty()) break;
            // switch
            switch_tab_list();
        }
    } while (0);
}
void shell_frontend::default_handler(char _c) {
    _front.push_back(_c);
    _M_write(_c);
    rewrite_back();
    cursor_move_back(_back.size());
}


bool shell_frontend::has_tab_next() {
    if (front_signature() == _tab_next_signature) return true;
    _tab_next.clear(); _tab_list.clear(); return false;
}
bool shell_frontend::has_tab_list() {
    if (front_signature() == _tab_list_signature) return true;
    _tab_next.clear(); _tab_list.clear(); return false;
}
void shell_frontend::switch_tab_list() {
    if (_tab_list.empty()) return;
    char _c; size_t _prev_tab_word_size = 0;
    while (true) {
        _tab_index = (_tab_index + 1) % _tab_list.size();
        // output
        fill_blank(_prev_tab_word_size + _back.size());
        cursor_move_back(_prev_tab_word_size + _back.size());
        const auto& _s = _tab_list.at(_tab_index);
        _M_write(_s);
        rewrite_back();
        cursor_move_back(_back.size() + _s.size());
        _prev_tab_word_size = _s.size();
        // read keyboard
        const auto _r = _M_read(_c);
        if (_r == -1) {
            if (errno == EINTR) continue;
            else {
                _end_of_file = true; return;
            }
        }
        else if (_r == 0) {
            _end_of_file = true; return;
        }
        if (_c != '\t') {
            _M_write(_s);
            _front.insert(_front.end(), _s.cbegin(), _s.cend());
            rewrite_back();
            cursor_move_back(_back.size());
            return;
        }
        else { // todo
            // if (_c == '\033') {
            //     _M_read(_c);
            //     _M_read(_c);
            //     if (_c == '3') _M_read(_c);
            // }
        }
    }

}
size_t shell_frontend::front_signature() const {
    size_t _seed = _front.size() * _front.size();
    for (const auto& _c : _front) {
        _seed ^= _c + 0x9e3779b9 + (_seed << 6) + (_seed >> 2);
    }
    return _seed;
}


void shell_frontend::cursor_move_back(size_t _n) {
    for (size_t _i = 0; _i < _n; ++_i) _M_write('\b');
}
void shell_frontend::fill_blank(size_t _n) {
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
void shell_frontend::_M_term_start_handler() {
    _M_raw();
    tcflush(_in, TCIFLUSH);
    tcgetattr(_in, &_default_in_setting);
    _shell_in_setting = _default_in_setting;
    _shell_in_setting.c_lflag &= ~ECHO;
    tcsetattr(_in, TCSANOW, &_shell_in_setting);
}
void shell_frontend::_M_term_end_handler() {
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