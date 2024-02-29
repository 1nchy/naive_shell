#include "shell.hpp"

#include <cassert>

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>

namespace asp {
shell::shell(shell_editor& _e)
 : _e(_e) {
    // _home_dir = std::filesystem::path(getenv("HOME"));
    // _cwd = std::filesystem::current_path();

    _cmd_symbol_dict.add(">");
    _cmd_symbol_dict.add(">>");
    _cmd_symbol_dict.add("<");
    // _cmd_symbol_dict.add("<<");
    _cmd_symbol_dict.add(";");
    _cmd_symbol_dict.add("|");
    _cmd_symbol_dict.add("||");
    _cmd_symbol_dict.add("&");
    _cmd_symbol_dict.add("&&");
}
shell::~shell() {

}
bool shell::wait() {
    size_t _k = 0; // instruction sequence
    std::string _word;
    std::string _relation;
    bool _in_ready = false; std::string _redirect_in; 
    bool _out_ready = false; std::string _redirect_out; short _redirect_out_type = 0;
    bool _need_further_input = false;
    bool _line_parsed = false;
    bool _background_mark = false;

    _e.show_information();
    reset();
    _e.show_prompt();
    std::string _s = _e.line();
    if (_s.empty()) return false;
    size_t _i = 0; // index of command string

    auto build_word = [&]() {
        if (_word.empty()) return;
        _need_further_input = false;
        if (_in_ready) {
            _redirect_in = _word;
            _in_ready = false;
        }
        else if (_out_ready) {
            _redirect_out = _word;
            _out_ready = false;
        }
        else {
            if (this->_parsed_command.size() <= _k) {
                this->_parsed_command.emplace_back();
            }
            assert(this->_parsed_command.size() == _k + 1);
            this->_parsed_command[_k].push_back(_word);
        }
        _word.clear();
    };
    auto build_instruction = [&]() {
        assert(this->_instruction_relation.size() == _k);
        assert(this->_instruction_redirection.size() == _k);
        this->_instruction_relation.emplace_back(_relation);
        this->_instruction_redirection.emplace_back(_redirect_in, _redirect_out);
        this->_instruction_redirection_type.emplace_back(_redirect_out_type);
        this->_background_instruction.emplace_back(_background_mark);
        ++_k;
        _relation.clear();
        _redirect_in.clear(); _redirect_out.clear(); _redirect_out_type = 0;
        _in_ready = false; _out_ready = false; _background_mark = false;
    };
    auto build_redirection = [&]() { // ignore the difference between < and <<
        if (_relation == "<" /*|| _relation == "<<"*/) {
            _in_ready = true;
        }
        else if (_relation == ">") {
            _out_ready = true;
            _redirect_out_type = 1;
        }
        else if (_relation == ">>") {
            _out_ready = true;
            _redirect_out_type = 2;
        }
    };
    auto end_of_line = [&]() {
        build_word();
        if (_need_further_input) {
            do {
            _e.show_prompt();
            _s = _e.line();
            } while (_s.empty());
            _i = 0;
            _line_parsed = false;
            return;
        }
        build_instruction();
        _line_parsed = true;
    };

    while (!_line_parsed) {
        // skip blank between words
        while (_i != _s.size() && _s[_i] == ' ') { ++_i; }
        if (_i == _s.size()) {
            end_of_line();
            continue;
        }

        for (; _i < _s.size(); ++_i) { // build one word in a cycle at least
            const auto& _c = _s[_i];
            if (_c == '\\') {
                ++_i;
                if (_i == _s.size() || _s[_i] == '\n' || _s[_i] == '\r') {
                    _e.show_prompt();
                    _s = _e.line();
                    if (_s.empty()) return false;
                    _i = 0;
                }
                _word.push_back(_s[_i]);
                continue;
            }
            else if (_c == '\'') {
                ++_i;
                while (true) {
                    if (_i == _s.size() || _s[_i] == '\n' || _s[_i] == '\r') {
                        _e.show_prompt();
                        _s = _e.line();
                        if (_s.empty()) return false;
                        _i = 0;
                    }
                    if (_s[_i] == '\'') {
                        // ++_i;
                        break;
                    }
                    _word.push_back(_s[_i++]);
                }
                continue;
            }
            // else if (_c == '\"') {}

            if (_c == ' ') { // build word
                build_word();
                break;
            }

            if (_c == '\n' || _c == '\r') { // build word and instruction // unused branch
                end_of_line(); 
                break;
            }

            size_t _l = _cmd_symbol_dict.longest_match(_s.cbegin() + _i, _s.cend());
            if (_l == 0) { // common char
                _word.push_back(_c);
                continue;
            }
            // 
            build_word();
            _relation = _s.substr(_i, _l);
            _i += _l;
            if (_relation == "|" || _relation == "||" || _relation == ";" || _relation == "&&") {
                build_instruction();
                _need_further_input = true;
                break;
            }
            else if (_relation == ">" || _relation  == ">>" || _relation == "<" /*|| _relation == "<<"*/) {
                build_redirection();
                _need_further_input = true;
                break;
            }
            else if (_relation == "&") {
                _background_mark = true;
                build_instruction();
                break;
            }
        }
    }
    return true;
}

bool shell::compile() { // extract
    return true;
}

bool shell::execute() {
    for (size_t _i = 0; _i < _parsed_command.size(); ++_i) {
        execute_instruction(_i);
    }
    return true;
}

void shell::reset() {
    _parsed_command.clear();
    _instruction_redirection.clear();
    _instruction_redirection_type.clear();
    _instruction_relation.clear();
}

size_t shell::execute_instruction(size_t _i) {
    const auto& _cmd = _parsed_command[_i];

    if (_internal_instruction.count(_cmd[0])) {
        // (_internal_instruction.at(_cmd[0])._handler)();
        const auto& _detail = _internal_instruction.at(_cmd[0]);
        if (_cmd.size() < _detail._min_args) {
            // error
            return 1;
        }
        if (_detail._max_args != 0 && _cmd.size() > _detail._max_args) {
            // error
            return 1;
        }
        (this->*_detail._handler)(_cmd);
        return 1;
    }

    pid_t _pid = fork();
    if (_pid == 0) { // child process
        // execl("echo", "hello", nullptr);
        std::vector<const char*> _cmd_args;
        for (const auto& _arg : _cmd) {
            _cmd_args.emplace_back(_arg.c_str());
        }
        _cmd_args.emplace_back(nullptr);

        // redirection


        execvp(_cmd[0].c_str(), const_cast<char* const*>(_cmd_args.data()));
        exit(EXIT_SUCCESS);
    }
    _current_processes.insert(_pid);
    waitpid(_pid, nullptr, WUNTRACED);
    _current_processes.erase(_pid);
    return 1;
}
size_t shell::execute_instruction_bg(size_t _i) {
    return 1;
}

void shell::cd(const std::vector<std::string>&) {}
void shell::cwd(const std::vector<std::string>&) {}
void shell::history(const std::vector<std::string>&) {}
void shell::quit(const std::vector<std::string>&) {}
void shell::bg(const std::vector<std::string>&) {}
void shell::job(const std::vector<std::string>&) {}
void shell::echo(const std::vector<std::string>&) {}
};