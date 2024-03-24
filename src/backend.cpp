#include "backend.hpp"

#include <sys/wait.h>

#include <chrono>
#include <thread>

#include "signal_stack/signal_stack.hpp"
#include "proc_status/proc_status.hpp"
#include "output/output.hpp"

namespace asp {

#define BUILT_IN_INSTRUCTION_ARGS_CHECK(_args) \
if (!builtin_instruction_check(__func__, _args)) return;

static const std::string _empty_string;
static signal_stack _signal_stack;
static shell_backend* _instance_pointer = nullptr;
static void shell_sigchld_handler(int _sig) {
    assert(_instance_pointer != nullptr);
    _instance_pointer->sigchld_handler(_sig);
}
static void shell_sigtstp_handler(int _sig) {
    assert(_instance_pointer != nullptr);
    _instance_pointer->sigtstp_handler(_sig);
}
static void shell_sigint_handler(int _sig) {
    assert(_instance_pointer != nullptr);
    _instance_pointer->sigint_handler(_sig);
}
static void shell_sigpipe_handler(int _sig) {
    assert(_instance_pointer != nullptr);
}
static void shell_sigttin_handler(int _sig) {
    assert(_instance_pointer != nullptr);
    _instance_pointer->sigttin_handler(_sig);
}
static void shell_sigttou_handler(int _sig) {
    assert(_instance_pointer != nullptr);
    _instance_pointer->sigttou_handler(_sig);
}


shell_backend::shell_backend(int _in, int _out, int _err)
: backend_interface(_in, _out, _err) {
    output::set_output_level(output::warn);
    _instance_pointer = this;

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

    dup2(_in, STDIN_FILENO);
    dup2(_out, STDOUT_FILENO);
    dup2(_err, STDERR_FILENO);

    _signal_stack.build(SIGCHLD, shell_sigchld_handler);
    _signal_stack.build(SIGTSTP, shell_sigtstp_handler);
    _signal_stack.build(SIGINT, shell_sigint_handler);
    _signal_stack.build(SIGTTIN, SIG_IGN);
    _signal_stack.build(SIGTTOU, SIG_IGN);

    clear();
}
shell_backend& shell_backend::singleton(int _in, int _out, int _err) {
    static shell_backend _instance(_in, _out, _err);
    return _instance;
}
shell_backend::~shell_backend() {

}
int shell_backend::parse(const std::string& _line) {
    if (_line.empty()) return -1;
    size_t _i = 0;
    while (_parse_status != parse_status::eof) {
        // handle parsing status
    if (_parse_status == parse_status::backslash) {}
    else if (_parse_status == parse_status::single_quote) {
        while (true) {
            if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                _parse_status == parse_status::single_quote;
                return 1;
            }
            if (_line[_i] == '\'') break;
            _word.push_back(_line[_i]); ++_i;
        }
    }
    // else if (_parse_status == parse_status::double_quotes) {}
    
        // skip blank between words
    while (_i != _line.size() && _line[_i] == ' ') { ++_i; }
    if (_i == _line.size()) {
        if (!end_of_line()) {
            return 1;
        }
        continue;
    }
        // build one word in a cycle at least
    for (; _i < _line.size(); ++_i) {
        const auto& _c = _line[_i];
        if (_c == '\\') {
            ++_i;
            if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                _parse_status = parse_status::backslash;
                return 1;
            }
            _word.push_back(_line[_i]);
        }
        else if (_c == '\'') {
            ++_i;
            while (true) {
                if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                    _parse_status == parse_status::single_quote;
                    return 1;
                }
                if (_line[_i] == '\'') break;
                _word.push_back(_line[_i]); ++_i;
            }
        }
        // else if (_c == '\"') {}
        else if (_c == ' ') {
            _commands.build_word(_word); _word.clear();
            break;
        }
        else if (_c == '\n' || _c == '\r') {
            if (!end_of_line()) return 1;
            break;
        }
        else { // character that should be read
    size_t _l = _cmd_symbol_dict.longest_match(_line.cbegin() + _i, _line.cend());
    if (_l == 0) { // normal character
        _word.push_back(_c);
        continue;
    }
        // special character
    _commands.build_word(_word); _word.clear();
    _relation = _line.substr(_i, _l); _i += _l;
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
    }}}}
    return 0;
}
bool shell_backend::compile() {
    if (_parse_status != parse_status::eof) {
        clear();
        return false;
    }
    return true;
}
bool shell_backend::execute() {
    for (size_t _i = 0; _i < _commands.size(); ++_i) {
        execute_command(_commands._parsed_command[_i]);
    }
    clear();
    return true;
}
std::string shell_backend::build_information() {
    std::string _info;
    _info.push_back('[');
    _info.append(_cwd.filename());
    size_t _fg_cnt = 0; size_t _bg_cnt = 0;
    for (const auto& [_p, _j] : _proc_map) {
        if (_j.foreground()) ++_fg_cnt;
        else ++_bg_cnt;
    }
    _info.append(std::string(" (") + std::to_string(_fg_cnt) + "," + std::to_string(_bg_cnt) + ")");
    _info.push_back(']');
    return _info;
}
std::vector<std::string> shell_backend::build_tab_list(const std::string&) {
    return {}; // todo
}
const std::string& shell_backend::prev_history() {
    if (_history.empty()) {
        return _empty_string;
    }
    if (_history_iterator != _history.cbegin()) {
        --_history_iterator;
    }
    return *_history_iterator;
}
const std::string& shell_backend::next_history() {
    if (_history.empty() || _history_iterator == _history.cend() || _history_iterator == _history.cend() - 1) {
        _history_iterator = _history.cend();
        return _empty_string;
    }
    ++_history_iterator;
    return *_history_iterator;
}
void shell_backend::append_history(const std::string& _s) {
    _history.push_back(_s);
    _history_iterator = _history.cend();
}


void shell_backend::execute_command(const command& _cmd) {
    const auto& _main_ins = _cmd._instructions.back();

    if (_cmd.size() == 1 && !_cmd._background && is_builtin_instruction(_main_ins.front())) {
        // redirect
        int _old_in = dup(_in);
        int _old_out = dup(_out);
        FILE* _input_file = nullptr; FILE* _output_file = nullptr;
        if (!_cmd._redirect_in.empty()) {
            _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
            if (_input_file != nullptr) {
                dup2(fileno(_input_file), _in);
            }
        }
        if (!_cmd._redirect_out.empty()) {
            if (_cmd._redirect_out_type == 1) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "w+");
                dup2(fileno(_output_file), _out);
            }
            else if (_cmd._redirect_out_type == 2) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "a+");
                dup2(fileno(_output_file), _out);
            }
        }
        execute_builtin_instruction(_main_ins);
        // re-redirect
        if (_input_file != nullptr) {
            fclose(_input_file);
            dup2(_old_in, _in);
        }
        if (_output_file != nullptr) {
            fclose(_output_file);
            dup2(_old_out, _in);
        }
        return;
    }

    _signal_stack.reset(SIGCHLD);
    _signal_stack.reset(SIGTSTP);
    _signal_stack.reset(SIGINT);
    _signal_stack.reset(SIGTTIN);
    _signal_stack.reset(SIGTTOU);

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
                dup2(_child_out_fd[1], _out);
                if (_i != _n) {
                    close(_child_in_fd[1]);
                    dup2(_child_in_fd[0], _in);
                }
                FILE* _input_file = nullptr;
                if (_i == _n) { // file redirection
                    if (!_cmd._redirect_in.empty()) {
                        _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
                        if (_input_file != nullptr) {
                            dup2(fileno(_input_file), _in);
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
                // ~~dup2(_child_out_fd[0], _editor.in())~~;
                /**
                 * 后续所有子进程的文件描述符都将继承该主子进程，因此对该主子进程文件描述符的复制应该放在最后
                */
                _temp_fd = _child_out_fd[0]; // 主子进程从该文件描述符读取输入
            }
            else {
                close(_child_out_fd[0]);
            }
            // _children_pipe.emplace_back(_child_out_fd[0]);

            _children.emplace_back(_pid);
        }
        if (_temp_fd != -1) {
            dup2(_temp_fd, _in);
        }

        FILE* _input_file = nullptr; FILE* _output_file = nullptr;
        if (_n < 2) {
            if (!_cmd._redirect_in.empty()) {
                _input_file = fopen(_cmd._redirect_in.c_str(), "r+");
                if (_input_file != nullptr) {
                    dup2(fileno(_input_file), _in);
                }
            }
        }
        if (!_cmd._redirect_out.empty()) {
            if (_cmd._redirect_out_type == 1) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "w+");
                dup2(fileno(_output_file), _out);
            }
            else if (_cmd._redirect_out_type == 2) {
                _output_file = fopen(_cmd._redirect_out.c_str(), "a+");
                dup2(fileno(_output_file), _out);
            }
        }

        execute_instruction(_main_ins);
        exit(EXIT_SUCCESS);
    }
    
    _signal_stack.restore(SIGCHLD);
    _signal_stack.restore(SIGTSTP);
    _signal_stack.restore(SIGINT);
    _signal_stack.restore(SIGTTIN);
    _signal_stack.restore(SIGTTOU);

    setpgid(_pid, _pid);
    job _j(_pid, _pid, _main_ins, _in, _out, _err);
    if (!_cmd._background) {
        _proc_map.insert({_pid, _j});
        int _status = 0;
        tcsetpgrp(_in, _j.pgid());
        tcsetpgrp(_out, _j.pgid());
        tcsetpgrp(_err, _j.pgid());
        while (waitpid(_pid, &_status, WSTOPPED) == -1 && errno == EINTR);
        waitpid_handler(_pid, _status);
    }
    else {
        printf("[%ld](%d) %s\n", _task_serial_i, _pid, _main_ins.front().c_str());
        _j.set_serial(_task_serial_i);
        _proc_map.insert({_pid, _j});
        append_background_job(_pid);
    }
    return;
}
void shell_backend::execute_instruction(const std::vector<std::string>& _args) {
    if (_builtin_instruction.contains(_args[0].c_str())) { // for built-in instruction
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
void shell_backend::execute_builtin_instruction(const std::vector<std::string>& _args) {
    (this->*_builtin_instruction.at(_args[0])._handler)(_args);
}


void shell_backend::sigchld_handler(int) {
    int _status = 0; pid_t _pid;
    while ((_pid = waitpid(-1, &_status, WNOHANG)) > 0) {
        output_debug("receive sigchld(%d)\n", _pid);
        waitpid_handler(_pid, _status);
    }
}
void shell_backend::sigtstp_handler(int) {
    for (const auto& [_p, _j] : this->_proc_map) {
        if (_j.foreground()) {
            output_debug("send SIGSTOP to %d\n", _j.pid());
            ::kill(-_j.pgid(), SIGSTOP);
        }
    }
}
void shell_backend::sigint_handler(int) {
    for (const auto& [_p, _j] : this->_proc_map) {
        if (_j.foreground()) {
            output_debug("send SIGSTOP to %d\n", _j.pid());
            ::kill(-_j.pgid(), SIGSTOP);
        }
    }
}
void shell_backend::sigpipe_handler(int) {}
void shell_backend::sigttin_handler(int) {
    output_debug("receive sigttin\n");
}
void shell_backend::sigttou_handler(int) {
    output_debug("receive sigttou\n");
}


void shell_backend::clear() {
    _commands.clear();
    _parse_status = parse_status::parsing;
    _word.clear(); _relation.clear();
    _history_iterator = _history.cend();
}
void shell_backend::waitpid_handler(pid_t _pid, int _status) {
    if (_pid == -1 || _pid == 0) return;

    if (_proc_map.contains(_pid)) {
    auto& _j = _proc_map.at(_pid);
    if (_j.foreground()) {
        output_debug("foreground(%d) with (%x)\n", _pid, _status);
        if (WIFEXITED(_status) || WIFSIGNALED(_status)) {
            _proc_map.erase(_pid);
        }
        else if (WIFSTOPPED(_status)) {
            setpgid(_pid, _pid);
            _j.set_serial(_task_serial_i);
            append_background_job(_pid);
            const std::vector<std::string> _state = {"Name"};
            const auto _vs = proc::get_status(_pid, _state);
            if (!_vs.empty()) {
                printf("(%d) \t%s stopped\n", _pid, _vs[0].c_str());
            }
        }
        tcsetpgrp(_in, getpid());
        tcsetpgrp(_out, getpid());
        tcsetpgrp(_err, getpid());
    }
    else {
        output_debug("background(%d) with (%x)\n", _pid, _status);
        if (WIFEXITED(_status) || WIFSIGNALED(_status)) {
            const std::vector<std::string> _state = {"Name"};
            const auto _vs = proc::get_status(_pid, _state);
            if (!_vs.empty()) {
                printf("[%ld](%d) \t%s done\n", _j.serial(), _pid, _vs[0].c_str());
            }
            remove_background_job(_j.serial());
            _proc_map.erase(_pid);
        }
        tcsetpgrp(_out, getpid());
        tcsetpgrp(_err, getpid());
    }
    }
}
size_t shell_backend::append_background_job(pid_t _pid) {
    _bg_map[_task_serial_i] = _pid;
    return _task_serial_i++;
}
pid_t shell_backend::remove_background_job(size_t _i) {
    const pid_t _pid = (_bg_map.contains(_i) ? _bg_map[_i] : -1);
    _bg_map.erase(_i);
    if (_bg_map.empty()) {
        _task_serial_i = 1;
    }
    return _pid;
}


bool shell_backend::builtin_instruction_check(const std::string& _cmd, const std::vector<std::string>& _args) {
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
bool shell_backend::is_builtin_instruction(const std::string& _cmd) const {
    return _builtin_instruction.contains(_cmd);
}
void shell_backend::pwd(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    printf("%s\n", _cwd.c_str());
}
void shell_backend::cd(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
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
void shell_backend::history(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    for (const auto& _s : _history) {
        printf("%s\n", _s.c_str());
    }
}
void shell_backend::quit(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    this->~shell_backend();
    exit(EXIT_FAILURE);
}
void shell_backend::bg(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const size_t _s = std::stoi(_args[1]);
    if (!_bg_map.contains(_s)) {
        printf("wrong task id\n");
        return;
    }
    const pid_t _pid = _bg_map[_s];
    auto& _j = _proc_map.at(_pid);
    ::kill(-_j.pgid(), SIGCONT);
}
void shell_backend::fg(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const size_t _s = std::stoi(_args[1]);
    if (!_bg_map.contains(_s)) {
        printf("wrong task id\n");
        return;
    }
    const pid_t _pid = _bg_map[_s];
    auto& _j = _proc_map.at(_pid);
    // tcsetattr(_editor.in(), TCSADRAIN, &_j._tmode);
    // tcsetattr(_editor.out(), TCSADRAIN, &_j._tmode);
    tcsetpgrp(_in, _j.pgid());
    tcsetpgrp(_out, _j.pgid());
    tcsetpgrp(_err, _j.pgid());
    ::kill(-_j.pgid(), SIGCONT);
    remove_background_job(_s);
    _j.set_serial();
    int _status;
    while (waitpid(_pid, &_status, WSTOPPED) == -1 && errno == EINTR);
    waitpid_handler(_pid, _status);
}
void shell_backend::jobs(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const std::vector<std::string> _states = {"Name", "State"};
    for (const auto& [_s, _p] : _bg_map) {
        const auto _vs = proc::get_status(_p, _states);
        if (!_vs.empty()) {
            printf("[%ld](%d) \t%s \t%s\n", _s, _p, _vs[0].c_str(), _vs[1].c_str());
        }
    }
}
void shell_backend::kill(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const size_t _i = std::stoi(_args[1]);
    if (!_bg_map.contains(_i)) {
        printf("wrong task id\n");
        return;
    }
    pid_t _pid = _bg_map[_i];
    auto& _j = _proc_map.at(_pid);
    ::kill(-_j.pgid(), SIGKILL);
    int _status;
    while (waitpid(_pid, &_status, WSTOPPED) == -1 && errno == EINTR);
    waitpid_handler(_pid, _status);
}
void shell_backend::echo(const std::vector<std::string>& _args) {}
void shell_backend::sleep(const std::vector<std::string>& _args) {
    BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const auto _x = std::stoi(_args[1]);
    std::this_thread::sleep_for(std::chrono::seconds(_x));
}

bool shell_backend::end_of_line(parse_status _s) {
    _commands.build_word(_word); _word.clear();
    if (_commands.further_input()) {
        // _line_parsed = false;
        _parse_status = _s;
        return false;
    }
    else {
        _commands.build_instruction();
        _commands.build_command(_relation);
        // _line_parsed = true;
        _parse_status = parse_status::eof;
        return true;
    }
}


void shell_backend::load_history() {
    // todo
    _history_iterator = _history.cend();
}
};