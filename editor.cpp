#include "editor.hpp"

#include <termios.h>

#include <algorithm>
#include <csignal>

namespace asp {
editor default_editor;

editor::editor(int _in, int _out, const std::string& _prompt)
 : _in(_in), _out(_out), _prompt(_prompt) {
    load_history();
}
editor::~editor() {
    
}
bool editor::wait() {
    clear();
    char _c; _history_pointer = _history.cend(); _end_of_file = false;
    // system("stty raw");
    _M_raw();

    termios _new_setting, _init_setting;
    tcgetattr(in(), &_init_setting);
    _new_setting = _init_setting;
    _new_setting.c_lflag &= ~ECHO;
    tcsetattr(in(), TCSANOW, &_new_setting);

    while (true) {
    const auto _r = _M_read(_c);
    if (_r < 0) {
        if (errno == EINTR) {
            _M_write("intr break read\n");
            continue;
            // return true;
        }
        else {
            return false;
        }
    }
    else if (_r == 0) {
        _end_of_file = true;
        tcsetattr(in(), TCSANOW, &_init_setting);
        // system("stty cooked");
        _M_cooked();
        _M_write('\n');
        return false;
    }

    if (_c == '\r' || _c == '\n') {
        write_back();
        if (empty()) continue;
        build_command();
        tcsetattr(in(), TCSANOW, &_init_setting);
        // system("stty cooked");
        _M_cooked();
        _M_write('\n');
        _history.emplace_back(_command);
        return true;
    }
    else if (_c == '\04' || _c == '\03') { // ctrl+c or ctrl+d
        write_back();
        _end_of_file = true;
        tcsetattr(in(), TCSANOW, &_init_setting);
        // system("stty cooked");
        _M_cooked();
        _M_write('\n');
        return false;
    }
    else if (_c == '\033') { // _c == '^['
        char _subc;
        _M_read(_subc);
        _M_read(_subc);
        if (_subc == 'A') { // up arrow ^[[A
            cursor_back(_front.size());
            fill_blank(0);
            cursor_back(_front.size() + _back.size());
            if (_history.empty()) {
                clear();
            }
            else if (_history_pointer == _history.cbegin()) {
                _M_write(*_history_pointer);
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
            else {
                --_history_pointer;
                _M_write(*_history_pointer);
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
        }
        else if (_subc == 'B') { // down arrow ^[[B
            cursor_back(_front.size());
            fill_blank(0);
            cursor_back(_front.size() + _back.size());
            if (_history.empty()
                || _history_pointer == _history.cend()
                || _history_pointer == _history.cend() - 1) {
                _history_pointer = _history.cend(); clear();
            }
            else {
                ++_history_pointer;
                _M_write(*_history_pointer);
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
        }
        else if (_subc == 'D') { // left arrow ^[[D
            if (!_front.empty()) {
                _back.push_back(_front.back());
                _front.pop_back();
                cursor_back(1);
            }
        }
        else if (_subc == 'C') { // right arrow ^[[C
            if (!_back.empty()) {
                write_back();
                _front.push_back(_back.back());
                _back.pop_back();
                cursor_back(_back.size());
            }
        }
        else if (_subc == 'H') { // home key ^[[H
            write_back();
            _back.insert(_back.end(), _front.crbegin(), _front.crend());
            _front.clear();
            cursor_back(_back.size());
        }
        else if (_subc == 'F') { // end key ^[[F
            write_back();
            _front.insert(_front.end(), _back.crbegin(), _back.crend());
            _back.clear();
        }
        else if (_subc == '3') { // del key ^[[3~
            _M_read(_subc); // get '~'
            if (!_back.empty()) {
                // viewer_back(5);
                _back.pop_back();
                write_back();
                fill_blank(1);
                cursor_back(_back.size() + 1);
            }
        }
    }
    else if (_c == 127) { // backspace
        if (!_front.empty()) {
            viewer_back(1);
            _front.pop_back();
            write_back();
            fill_blank(1);
            cursor_back(_back.size() + 1);
        }
    }
    else if (_c == '\t') {
        
    }
    else {
        _front.push_back(_c);
        _M_write(_c);
        write_back();
        cursor_back(_back.size());
    }
    }

    // system("stty cooked");
    _M_cooked();
    tcsetattr(in(), TCSANOW, &_init_setting);
    return true;
}
const std::string& editor::line() {
    if (!wait()) _command.clear();
    return _command;
}
void editor::back(size_t _n) {
    if (_n == 0 || _n >= _front.size()) {
        _front.clear();
    }
    else {
        for (size_t _i = 0; _i < _n; ++_i) {
            _front.pop_back();
        }
    }
}
void editor::del(size_t _n) {
    if (_n == 0 || _n >= _back.size()) {
        _back.clear();
    }
    else {
        for (size_t _i = 0; _i < _n; ++_i) {
            _back.pop_back();
        }
    }
}
void editor::cursor_back(size_t _n) {
    for (size_t _i = 0; _i < _n; ++_i) _M_write('\b');
}
void editor::viewer_back(size_t _n) {
    cursor_back(_n);
    fill_blank(_n);
    cursor_back(_n);
}
void editor::fill_blank(size_t _n) {
    if (_n == 0) { _n = _front.size() + _back.size(); }
    for (size_t _i = 0; _i < _n; ++_i) {
        _M_write(' ');
    }
}
void editor::write_front() {
    std::for_each(_front.cbegin(), _front.cend(), [this](char _c) {
        _M_write(_c);
    });
}
void editor::write_back(size_t _i) {
    if (_i == 0) _i = _back.size();
    for (size_t _j = _back.size() - 1; _j >= 0 && _i > 0; --_j, --_i) {
        _M_write(_back[_j]);
    }
}
void editor::tab() {}

void editor::load_history() {}

void editor::_M_cooked() {
    _local_ss.build(SIGCHLD, nullptr);
    system("stty cooked");
    _local_ss.restore(SIGCHLD);
}
void editor::_M_raw() {
    _local_ss.build(SIGCHLD, nullptr);
    system("stty raw");
    _local_ss.restore(SIGCHLD);
}

void editor::build_command() {
    _command.clear();
    _command.reserve(_front.size() + _back.size());
    _command.append(_front.cbegin(), _front.cend());
    _command.append(_back.crbegin(), _back.crend());
}

};