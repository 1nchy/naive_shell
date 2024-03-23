#include "shell.hpp"

#include <cassert>

#include <cstdlib>
#include <csignal>
// #include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>

#include <chrono>
#include <thread>

#include "proc_status.hpp"
#include "signal_stack.hpp"

#include "editor.hpp"

asp::signal_stack _ss;

namespace asp {

signal_stack _ss;
shell shell_singleton(asp::default_editor);

void shell_sigchld_handler(int _sig) {
    shell_singleton.sigchld_handler(_sig);
}
void shell_sigtstp_handler(int _sig) {
    shell_singleton.sigtstp_handler(_sig);
}
void shell_sigint_handler(int _sig) {
    shell_singleton.sigint_handler(_sig);
}
void shell_sigpipe_handler(int _sig) {

}
void shell_sigttin_handler(int _sig) {
    shell_singleton.sigttin_handler(_sig);
}
void shell_sigttou_handler(int _sig) {
    shell_singleton.sigttou_handler(_sig);
}

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

    _ss.build(SIGCHLD, shell_sigchld_handler);
    _ss.build(SIGTSTP, shell_sigtstp_handler);
    // _ss.build(SIGSTOP, shell_sigtstp_handler); // tbd
    _ss.build(SIGINT, shell_sigint_handler);
    // _ss.build(SIGTTIN, shell_sigttin_handler);
    _ss.build(SIGTTIN, SIG_IGN);
    // _ss.build(SIGTTOU, shell_sigttou_handler);
    _ss.build(SIGTTOU, SIG_IGN);
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
    line();
    if (_line.empty()) return false;
    size_t _i = 0; // index of command string

    auto end_of_line = [&]() {
        this->_commands.build_word(_word); _word.clear();
        if (this->_commands.further_input()) {
            do {
                _editor.show_prompt();
                line();
            } while (_line.empty());
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
        while (_i != _line.size() && _line[_i] == ' ') { ++_i; }
        if (_i == _line.size()) {
            end_of_line();
            continue;
        }

        for (; _i < _line.size(); ++_i) { // build one word in a cycle at least
            const auto& _c = _line[_i];
            if (_c == '\\') {
                ++_i;
                if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                    _editor.show_prompt();
                    line();
                    if (_line.empty()) return false;
                    _i = 0;
                }
                _word.push_back(_line[_i]);
                continue;
            }
            else if (_c == '\'') {
                ++_i;
                while (true) {
                    if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                        _editor.show_prompt();
                        line();
                        if (_line.empty()) return false;
                        _i = 0;
                    }
                    if (_line[_i] == '\'') {
                        // ++_i;
                        break;
                    }
                    _word.push_back(_line[_i++]);
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

            size_t _l = _cmd_symbol_dict.longest_match(_line.cbegin() + _i, _line.cend());
            if (_l == 0) { // common char
                _word.push_back(_c);
                continue;
            }
            // 
            _commands.build_word(_word); _word.clear();
            _relation = _line.substr(_i, _l);
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

void shell::sigchld_handler(int) {
    while (1) {
        int _status = 0;
        pid_t _pid = waitpid(-1, &_status, WNOHANG);
        if (_pid == -1 || _pid == 0) break;
        printf("receive sigchld(%d)\n", _pid);
        waitpid_handler(_pid, _status);
    }
}
void shell::sigtstp_handler(int) {
    for (const auto& [_p, _j] : this->_proc_map) {
        if (_j.foreground()) {
            printf("send SIGSTOP to %d\n", _j.pid());
            ::kill(-_j.pgid(), SIGSTOP);
        }
    }
}
void shell::sigint_handler(int) {
    for (const auto& [_p, _j] : this->_proc_map) {
        if (_j.foreground()) {
            printf("send SIGINT to %d\n", _j.pid());
            ::kill(-_j.pgid(), SIGINT);
        }
    }
}
void shell::sigpipe_handler(int) {

}
void shell::sigttin_handler(int) {
    printf("receive sigttin\n");
    // ::kill(getpid(), SIGCONT);
}
void shell::sigttou_handler(int) {
    printf("receive sigttou\n");
    // ::kill(getpid(), SIGCONT);
}

void shell::line() {
    // _ss.reset(SIGCHLD);
    _line = _editor.line();
    // _ss.restore(SIGCHLD);
}


void shell::reset() {
    _commands.reset();
}

void shell::execute_command(const command& _cmd) {
    const auto& _main_ins = _cmd._instructions.back();

    if (_cmd.size() == 1 && !_cmd._background && is_builtin_instruction(_main_ins.front())) {
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

    _ss.reset(SIGCHLD); // tbd
    _ss.reset(SIGTSTP);
    // _ss.reset(SIGSTOP); // tbd
    _ss.reset(SIGINT);
    _ss.reset(SIGTTIN);
    _ss.reset(SIGTTOU);

    pid_t _pid = fork();
    if (_pid == 0) { // main child process
        const pid_t _main_pid = getpid();
        setpgid(_main_pid, _main_pid);
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
                setpgid(0, _main_pid);
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
                // if (_input_file != nullptr) fclose(_input_file);
                exit(EXIT_SUCCESS);
            }
            setpgid(_pid, _main_pid);
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

        // for (const auto& _i : _children) {
        //     while (true) {
        //     const auto _r = waitpid(_i, nullptr, WUNTRACED);
        //     if (_r < 0) {
        //         if (errno == EINTR) {
        //             printf("waitpid interupted.\n");
        //         }
        //     }
        //     break;
        //     }
        // }
        // if (_input_file != nullptr) { fclose(_input_file); }
        // if (_output_file != nullptr) { fclose(_output_file); }
        exit(EXIT_SUCCESS);
    }
    
    _ss.restore(SIGCHLD); // tbd
    _ss.restore(SIGTSTP);
    // _ss.restore(SIGSTOP); // tbd
    _ss.restore(SIGINT);
    _ss.restore(SIGTTIN);
    _ss.restore(SIGTTOU);

    setpgid(_pid, _pid);
    job _j(_pid, _pid, _main_ins, _editor.in(), _editor.out());
    if (!_cmd._background) {
        _proc_map.insert({_pid, _j});
        int _status = 0;
        tcsetpgrp(_editor.in(), _j.pgid());
        tcsetpgrp(_editor.out(), _j.pgid());
        // waitpid(_pid, &_status, WUNTRACED); // todo
        pid_t _r = waitpid(_pid, &_status, WSTOPPED);
        if (_r < 0) {
            if (errno == EINTR) {
                printf("waitpid interupted.\n");
            }
        }
        else {
            waitpid_handler(_pid, _status);
        }
    }
    else {
        // tcsetpgrp(_editor.out(), _j.pgid());
        printf("[%ld] %d %s\n", _task_serial_i, _pid, _main_ins.front().c_str());
        _j.set_serial(_task_serial_i);
        _proc_map.insert({_pid, _j});
        _bg_map[_task_serial_i++] = _pid;
    }
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
    size_t _fg_cnt = 0; size_t _bg_cnt = 0;
    for (const auto& [_p, _j] : _proc_map) {
        if (_j.foreground()) ++_fg_cnt;
        else ++_bg_cnt;
    }
    _info.append(std::string(" fg(") + std::to_string(_fg_cnt) + ") bg(" + std::to_string(_bg_cnt) + ")");
    _info.push_back(']');
    return _info;
}

void shell::cd(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("cd", _args)) {
        return;
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
void shell::pwd(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("pwd", _args)) {
        return;
    }
    printf("%s\n", _cwd.c_str());
}
void shell::history(const std::vector<std::string>& _args) {}
void shell::quit(const std::vector<std::string>& _args) {
    exit(EXIT_FAILURE);
}
void shell::bg(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("bg", _args)) {
        exit(EXIT_FAILURE);
    }
    const size_t _s = std::stoi(_args[1]);
    if (!_bg_map.count(_s)) {
        printf("wrong task id\n");
        return;
    }
    const pid_t _pid = _bg_map[_s];
    auto& _j = _proc_map.at(_pid);
    ::kill(-_j.pgid(), SIGCONT);
}
void shell::fg(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("fg", _args)) {
        exit(EXIT_FAILURE);
    }
    const size_t _s = std::stoi(_args[1]);
    if (!_bg_map.count(_s)) {
        printf("wrong task id\n");
        return;
    }
    const pid_t _pid = _bg_map[_s];
    auto& _j = _proc_map.at(_pid);
    printf("tcsetpgrp(%d)\n", _j.pgid());
    // tcsetattr(_editor.in(), TCSADRAIN, &_j._tmode);
    // tcsetattr(_editor.out(), TCSADRAIN, &_j._tmode);
    tcsetpgrp(_editor.in(), _j.pgid());
    tcsetpgrp(_editor.out(), _j.pgid());
    ::kill(-_j.pgid(), SIGCONT);
    _bg_map.erase(_s);
    _j.set_serial();
    int _status;
    while (1) {
    pid_t _r = waitpid(_pid, &_status, WSTOPPED);
    if (_r == -1 && errno == EINTR) {
        printf("waitpid interupted.\n"); continue;
    }
    waitpid_handler(_pid, _status); break;
    }
}
void shell::jobs(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("jobs", _args)) {
        return;
    }
    printf("job list:\n");
    const std::vector<std::string> _states = {"Name", "State"};
    for (const auto& [_s, _p] : _bg_map) {
        const auto _vs = proc::get_status(_p, _states);
        if (!_vs.empty()) {
            printf("[%ld](%d) \t%s \t%s\n", _s, _p, _vs[0].c_str(), _vs[1].c_str());
        }
    }
}
void shell::kill(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("kill", _args)) {
        return;
    }
    const size_t _i = std::stoi(_args[1]);
    if (!_bg_map.count(_i)) {
        printf("wrong task id\n");
        return;
    }
    pid_t _pid = _bg_map[_i];
    auto& _j = _proc_map.at(_pid);
    ::kill(-_j.pgid(), SIGINT);
    _bg_map.erase(_i);
    _proc_map.erase(_pid);
    int _status;
    while (1) {
    pid_t _r = waitpid(_pid, &_status, WSTOPPED);
    if (_r == -1 && errno == EINTR) {
        printf("waitpid interupted.\n"); continue;
    }
    waitpid_handler(_pid, _status); break;
    }
}
void shell::echo(const std::vector<std::string>& _args) {}
void shell::sleep(const std::vector<std::string>& _args) {
    if (!internal_instruction_check("sleep", _args)) {
        return;
    }
    const auto _x = std::stoi(_args[1]);
    std::this_thread::sleep_for(std::chrono::seconds(_x));
}

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

size_t shell::search_in_background(pid_t _pid) {
    for (const auto& _i : this->_bg_map) {
        if (_i.second == _pid) return _i.first;
    }
    return 0;
}

void shell::waitpid_handler(pid_t _pid, int _status) {
    if (_pid == -1 || _pid == 0) return;

    if (_proc_map.count(_pid)) {
    auto& _j = _proc_map.at(_pid);
    if (_j.foreground()) {
        printf("foreground(%d) with (%x)\n", _pid, _status);
        if (WIFEXITED(_status) || WIFSIGNALED(_status)) {
            _proc_map.erase(_pid);
        }
        else if (WIFSTOPPED(_status)) {
            setpgid(_pid, _pid);
            _j.set_serial(_task_serial_i);
            _bg_map[_task_serial_i++] = _pid;
            const std::vector<std::string> _state = {"Name"};
            const auto _vs = proc::get_status(_pid, _state);
            if (!_vs.empty()) {
                printf("(%d) \t%s stopped\n", _pid, _vs[0].c_str());
            }
        }
        tcsetpgrp(_editor.in(), getpid());
        tcsetpgrp(_editor.out(), getpid());
    }
    else {
        printf("background(%d) with (%x)\n", _pid, _status);
        if (WIFEXITED(_status) || WIFSIGNALED(_status)) {
            const std::vector<std::string> _state = {"Name"};
            const auto _vs = proc::get_status(_pid, _state);
            if (!_vs.empty()) {
                printf("[%ld](%d) \t%s done\n", _j.serial(), _pid, _vs[0].c_str());
            }
            _bg_map.erase(_j.serial());
            _proc_map.erase(_pid);
        }
        tcsetpgrp(_editor.out(), getpid());
    }
    }
}

};