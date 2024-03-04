#include "shell.hpp"

#include <cassert>

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>

namespace asp {
shell::shell(shell_editor& _e)
 : _editor(_e) {
    _home_dir = std::filesystem::path(getenv("HOME"));
    _cwd = _prev_cwd = std::filesystem::current_path();

    _cmd_symbol_dict.add(">");
    _cmd_symbol_dict.add(">>");
    _cmd_symbol_dict.add("<");
    // _cmd_symbol_dict.add("<<");
    _cmd_symbol_dict.add(";");
    _cmd_symbol_dict.add("|");
    _cmd_symbol_dict.add("||");
    _cmd_symbol_dict.add("&");
    _cmd_symbol_dict.add("&&");

    dup2(_editor.in(), fileno(stdin));
    dup2(_editor.out(), fileno(stdout));
}
shell::~shell() {

}
bool shell::wait() {
    std::string _word;
    std::string _relation;
    bool _line_parsed = false;

    _editor.show_information(build_information());
    reset();
    _editor.show_prompt();
    std::string _s = _editor.line();
    if (_s.empty()) return false;
    size_t _i = 0; // index of command string

    auto end_of_line = [&]() {
        this->_commands.build_word(_word); _word.clear();
        if (this->_commands.further_input()) {
            do {
            _editor.show_prompt();
            _s = _editor.line();
            } while (_s.empty());
            _i = 0;
            _line_parsed = false;
            return;
        }
        this->_commands.build_instruction();
        this->_commands.build_command(_relation);
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
                    _editor.show_prompt();
                    _s = _editor.line();
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
                        _editor.show_prompt();
                        _s = _editor.line();
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
                _commands.build_word(_word); _word.clear();
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
            _commands.build_word(_word); _word.clear();
            _relation = _s.substr(_i, _l);
            _i += _l;
            if (_relation == "|") {
                _commands.build_instruction();
                _commands.need_further_input();
                break;
            }
            else if (_relation == "||" || _relation == ";" || _relation == "&&") {
                _commands.build_instruction();
                _commands.build_command(_relation);
                _commands.need_further_input();
                break;
            }
            else if (_relation == ">" || _relation  == ">>" || _relation == "<" /*|| _relation == "<<"*/) {
                _commands.prepare_redirection(_relation);
                _commands.need_further_input();
                break;
            }
            else if (_relation == "&") {
                _relation.clear();
                _commands.build_instruction();
                _commands.build_command(_relation, true);
                break;
            }
        }
    }
    return true;
}

bool shell::compile() { // further check and extract
    return true;
}

bool shell::execute() {
    for (size_t _i = 0; _i < _commands.size(); ++_i) {
        execute_command(_commands._parsed_command[_i]);
    }
    return true;
}

void shell::reset() {
    _commands.reset();
}

void shell::execute_command(const command& _cmd) {
    const auto& _main_ins = _cmd._instructions.back();

    if (_cmd.size() == 1 && is_builtin_instruction(_main_ins.front())) {
        // redirect
        int _old_in = dup(_editor.in());
        int _old_out = dup(_editor.out());
        FILE* _input_file = nullptr; FILE* _output_file = nullptr;
        if (!_cmd._redirect_in.empty()) {
            _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
            if (_input_file != nullptr) {
                dup2(fileno(_input_file), _editor.in());
            }
        }
        if (!_cmd._redirect_out.empty()) {
            if (_cmd._redirect_out_type == 1) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "w+");
                dup2(fileno(_output_file), _editor.out());
            }
            else if (_cmd._redirect_out_type == 2) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "a+");
                dup2(fileno(_output_file), _editor.out());
            }
        }
        execute_builtin_instruction(_main_ins);
        // re-redirect
        if (_input_file != nullptr) {
            fclose(_input_file);
            dup2(_old_in, _editor.in());
        }
        if (_output_file != nullptr) {
            fclose(_output_file);
            dup2(_old_out, _editor.out());
        }
        return;
    }

    pid_t _pid = fork();
    if (_pid == 0) { // main child process
        std::vector<pid_t> _children;
        // std::vector<int> _children_pipe;
        int _child_in_fd[2]; int _child_out_fd[2];
        int _temp_fd = -1;
        const size_t _n = _cmd.size();
        if (_n >= 2) {
            pipe(_child_in_fd); // pipe between main child process and 1st process
        }
        for (size_t _i = 2; _i <= _n; ++_i) {
            std::swap(_child_in_fd, _child_out_fd);
            if (_i != _n) {
                pipe(_child_in_fd);
            }
            _pid = fork();
            if (_pid == 0) { // other process
                const auto& _ins = _cmd._instructions[_n - _i];
                // redirection
                close(_child_out_fd[0]);
                dup2(_child_out_fd[1], _editor.out());
                if (_i != _n) {
                    close(_child_in_fd[1]);
                    dup2(_child_in_fd[0], _editor.in());
                }
                FILE* _input_file = nullptr;
                if (_i == _n) { // file redirection
                    if (!_cmd._redirect_in.empty()) {
                        _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
                        if (_input_file != nullptr) {
                            dup2(fileno(_input_file), _editor.in());
                        }
                    }
                }

                execute_instruction(_ins);
                // execvp(_cmd[0].c_str(), const_cast<char* const*>(_cmd_args.data()));
                if (_input_file != nullptr) fclose(_input_file);
                exit(EXIT_SUCCESS);
            }
            // close out fd
            close(_child_out_fd[1]);
            if (_i == 2) {
                // dup2(_child_out_fd[0], _editor.in());
                /**
                 * 后续所有子进程的文件描述符都将继承该主子进程，因此对该主子进程文件描述符的复制应该放在最后
                */
                _temp_fd = _child_out_fd[0];
            }
            else {
                close(_child_out_fd[0]);
            }
            // _children_pipe.emplace_back(_child_out_fd[0]);

            _children.emplace_back(_pid);
        }
        if (_temp_fd != -1) {
            dup2(_temp_fd, _editor.in());
        }

        FILE* _input_file = nullptr; FILE* _output_file = nullptr;
        if (_n < 2) {
            if (!_cmd._redirect_in.empty()) {
                _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
                if (_input_file != nullptr) {
                    dup2(fileno(_input_file), _editor.in());
                }
            }
        }
        if (!_cmd._redirect_out.empty()) {
            if (_cmd._redirect_out_type == 1) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "w+");
                dup2(fileno(_output_file), _editor.out());
            }
            else if (_cmd._redirect_out_type == 2) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "a+");
                dup2(fileno(_output_file), _editor.out());
            }
        }

        execute_instruction(_main_ins);

        for (const auto& _i : _children) {
            waitpid(_i, nullptr, WUNTRACED);
        }
        // for (const auto& _i : _children_pipe) {
        //     close(_i);
        // }
        if (_input_file != nullptr) { fclose(_input_file); }
        if (_output_file != nullptr) { fclose(_output_file); }
        exit(EXIT_SUCCESS);
    }
    _current_processes.insert(_pid);
    waitpid(_pid, nullptr, WUNTRACED);
    _current_processes.erase(_pid);
    return;
}

void shell::execute_instruction(const std::vector<std::string>& _args) {
    if (_builtin_instruction.count(_args[0].c_str())) { // for built-in instruction
        execute_builtin_instruction(_args);
        return;
    }
    else { // for other instruction
        std::vector<const char*> _cmd_args;
        for (const auto& _arg : _args) {
            _cmd_args.emplace_back(_arg.c_str());
        }
        _cmd_args.emplace_back(nullptr);
        execvp(_args[0].c_str(), const_cast<char* const*>(_cmd_args.data()));
        return;
    }
}
void shell::execute_builtin_instruction(const std::vector<std::string>& _args) {
    (this->*_builtin_instruction.at(_args[0])._handler)(_args);
}

std::string shell::build_information() {
    std::string _info;
    _info.push_back('[');
    _info.append(_cwd.filename());
    _info.push_back(']');
    return _info;
}

void shell::cd(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("cd", _args)) {
        exit(EXIT_FAILURE);
    }
    if (_args.size() == 1) { // cd
        chdir(_home_dir.c_str());
    }
    else if (_args.at(1) == "-") { // cd -
        chdir(_prev_cwd.c_str());
        printf("%s\n", _prev_cwd.c_str());
    }
    else if (_args.at(1) == ".") { // cd .
        return;
    }
    else if (_args.at(1) == "..") { // cd ..
        chdir(_cwd.parent_path().c_str());
    }
    else {
        chdir(_args.at(1).c_str());
    }
    _prev_cwd = _cwd;
    _cwd = std::filesystem::current_path();
}
void shell::cwd(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("cwd", _args)) {
        exit(EXIT_FAILURE);
    }
    // const auto& _cwd_str = _cwd.string();
    printf("%s\n", _cwd.c_str());
    // printf("%s\n", getcwd(nullptr, 0));
    // write(stdout, _cwd_str.c_str(), _cwd_str.size());
}
void shell::history(const std::vector<std::string>& _args) {}
void shell::quit(const std::vector<std::string>& _args) {}
void shell::bg(const std::vector<std::string>& _args) {}
void shell::job(const std::vector<std::string>& _args) {}
void shell::echo(const std::vector<std::string>& _args) {}

bool shell::internal_instruction_check(const std::string& _cmd, const std::vector<std::string>& _args) {
    if (!is_builtin_instruction(_cmd)) return false;
    const auto& _details = _builtin_instruction.at(_cmd);
    if (_args.size() < _details._min_args) {
        return false;
    }
    if (_details._max_args != 0 && _args.size() > _details._max_args) {
        return false;
    }
    if (_args[0] != _cmd) {
        return false;
    }
    return true;
}
bool shell::is_builtin_instruction(const std::string& _cmd) const {
    return _builtin_instruction.count(_cmd);
}
};