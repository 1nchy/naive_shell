#include "editor.hpp"

#include <algorithm>

namespace asp {
// editor::editor(std::istream& _is, std::ostream& _os, const std::string& _prompt)
//  : _is(_is), _os(_os), _prompt(_prompt) {
//     load_history();
// }
editor::editor(int _in, int _out, const std::string& _prompt)
 : _in(_in), _out(_out), _prompt(_prompt) {

}
editor::~editor() {
    
}
bool editor::wait() {
    // out() << _prompt;
    clear();
    /*int*/char _c; _history_pointer = _history.cend(); _end_of_file = false;
    system("stty raw");

    // while ((_c = in().get()) != EOF) {
    while (true) {
    read(_in, &_c, sizeof(_c));


    if (_c == '\r' || _c == '\n') {
        viewer_back(2);
        write_back();
        if (empty()) continue;
        build_command();
        system("stty cooked");
        // out() << std::endl;
        write(_out, "\n", 1);
        _history.emplace_back(_command);
        return true;
    }
    else if (_c == '\04' || _c == '\03') { // ctrl+c or ctrl+d
        viewer_back(2);
        _end_of_file = true;
        system("stty cooked");
        // out() << std::endl;
        write(_out, "\n", 1);
        return false;
    }
    else if (_c == '\033') { // _c == '^['
        // in().get(); // '['
        // /*int*/char _subc = in().get();
        char _subc;
        read(_in, &_subc, sizeof(_subc));
        read(_in, &_subc, sizeof(_subc));
        if (_subc == 'A') { // up arrow ^[[A
            viewer_back(4);
            cursor_back(_front.size());
            fill_blank(0);
            cursor_back(_front.size() + _back.size());
            if (_history.empty()) {
                clear();
            }
            else if (_history_pointer == _history.cbegin()) {
                // out() << *_history_pointer;
                write(_out, _history_pointer->data(), _history_pointer->size() * sizeof(char));
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
            else {
                --_history_pointer;
                // out() << *_history_pointer;
                write(_out, _history_pointer->data(), _history_pointer->size() * sizeof(char));
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
        }
        else if (_subc == 'B') { // down arrow ^[[B
            viewer_back(4);
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
                // out() << *_history_pointer;
                write(_out, _history_pointer->data(), _history_pointer->size() * sizeof(char));
                clear();
                _front.assign(_history_pointer->cbegin(), _history_pointer->cend());
            }
        }
        else if (_subc == 'D') { // left arrow ^[[D
            viewer_back(4);
            write_back();
            cursor_back(_back.size());
            if (!_front.empty()) {
                _back.push_back(_front.back());
                _front.pop_back();
                cursor_back(1);
            }
        }
        else if (_subc == 'C') { // right arrow ^[[C
            viewer_back(4);
            if (!_back.empty()) {
                write_back();
                _front.push_back(_back.back());
                _back.pop_back();
                cursor_back(_back.size());
            }
        }
        else if (_subc == 'H') { // home key ^[[H
            viewer_back(4);
            write_back();
            _back.insert(_back.end(), _front.crbegin(), _front.crend());
            _front.clear();
            cursor_back(_back.size());
        }
        else if (_subc == 'F') { // end key ^[[F
            viewer_back(4);
            write_back();
            _front.insert(_front.end(), _back.crbegin(), _back.crend());
            _back.clear();
        }
        else if (_subc == '3') { // del key ^[[3~
            // in().get(); // get '~'
            read(_in, &_subc, sizeof(_subc));
            if (!_back.empty()) {
                viewer_back(5);
                _back.pop_back();
                write_back();
                fill_blank(1);
                cursor_back(_back.size() + 1);
            }
            else {
                viewer_back(5);
            }
        }
    }
    else if (_c == 127) { // backspace
        if (!_front.empty()) {
            viewer_back(3);
            _front.pop_back();
            write_back();
            fill_blank(1);
            cursor_back(_back.size() + 1);
        }
        else {
            viewer_back(2);
        }
    }
    else {
        // if (_c == ' ' && _front.empty()) {
        //     viewer_back(1);
        //     if (!_back.empty()) {
        //         out() << _back.back();
        //         viewer_back(1);
        //     }
        //     continue;
        // }
        _front.push_back(_c);
        write_back();
        cursor_back(_back.size());
    }
    }

    system("stty cooked");
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
    // for (size_t _i = 0; _i < _n; ++_i) printf("\b");
    for (size_t _i = 0; _i < _n; ++_i) write(_out, "\b", 1);
}
void editor::viewer_back(size_t _n) {
    cursor_back(_n);
    fill_blank(_n);
    cursor_back(_n);
}
void editor::viewer_del(size_t _n) {
    // todo
}
void editor::fill_blank(size_t _n) {
    // printf("%*s", int(_n == 0 ? _front.size() + _back.size() : _n), "");
    if (_n == 0) { _n = _front.size() + _back.size(); }
    for (size_t _i = 0; _i < _n; ++_i) {
        write(_out, " ", 1);
    }
}
void editor::write_front() {
    std::for_each(_front.cbegin(), _front.cend(), [this](char _c) {
        // _os << _c;
        write(_out, &_c, sizeof(char));
    });
}
void editor::write_back(size_t _i) {
    if (_i == 0) _i = _back.size();
    for (size_t _j = _back.size() - 1; _j >= 0 && _i > 0; --_j, --_i) {
        write(_out, &_back[_j], sizeof(char));
    }
    // std::for_each(_back.crbegin(), _back.crend(), [this](char _c) {
    //     // _os << _c;
    //     write(_out, &_c, sizeof(char));
    // });
}
void editor::tab() {}

void editor::load_history() {}
void editor::build_command() {
    _command.resize(_front.size() + _back.size());
    std::merge(_front.cbegin(), _front.cend(), _back.crbegin(), _back.crend(), _command.begin());
}

};