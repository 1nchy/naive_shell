#include "backend.hpp"

#include <sys/wait.h>

#include <chrono>
#include <thread>

#include <unistd.h>

#include "signal_stack/signal_stack.hpp"
#include "proc_status/proc_status.hpp"
#include "file_system/file_system.hpp"
#include "output/output.hpp"

extern char** environ;

namespace asp {

// #define BUILT_IN_INSTRUCTION_ARGS_CHECK(_args) \
// if (!builtin_instruction_check(__func__, _args)) return;

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
    output::set_output_level(output::fatal);
    output_debug("shell_backend::constructor\n");
    _instance_pointer = this;

    _home_dir = std::filesystem::path(getenv("HOME"));
    _cwd = _prev_cwd = std::filesystem::current_path();

    init_env_variable_map();
    fetch_env_dict();
    fetch_program_dict();

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
    output_debug("shell_backend::destructor\n");
}
int shell_backend::parse(const std::string& _line) {
    // if (_line.empty()) return 0;
    size_t _i = 0;
    while (_parse_status != parse_status::eof) {
        // handle parsing status
    if (_parse_status == parse_status::backslash) {
        if (_i == _line.size()) {
            _parse_status = parse_status::backslash;
            return 1;
        }
        _parse_status = parse_status::parsing;
        _word.push_back(_line[_i]); ++_i;
    }
    else if (_parse_status == parse_status::single_quote) {
        while (true) {
            if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                _parse_status = parse_status::single_quote;
                return 1;
            }
            _word.push_back(_line[_i]); ++_i;
            if (_word.back() == '\'') {
                _parse_status = parse_status::parsing;
                break;
            }
        }
    }
    // else if (_parse_status == parse_status::double_quotes) {}
    // else if (_parse_status == parse_status::back_quotes) {}
    
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
            _word.push_back(_c); ++_i;
            if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                _parse_status = parse_status::backslash;
                return 1;
            }
            _word.push_back(_line[_i]);
        }
        else if (_c == '\'') {
            _word.push_back(_c); ++_i;
            while (true) {
                if (_i == _line.size() || _line[_i] == '\n' || _line[_i] == '\r') {
                    _parse_status = parse_status::single_quote;
                    return 1;
                }
                _word.push_back(_line[_i]); ++_i;
                if (_word.back() == '\'') {
                    _parse_status = parse_status::parsing;
                    break;
                }
            }
        }
        // else if (_c == '\"') {}
        // else if (_c == '`') {}
        else if (_c == ' ') {
            _commands.build_word(_word); _word.clear();
            break;
        }
        else if (_c == '\n' || _c == '\r') {
            if (!end_of_line()) return 1;
            break;
        }
        else { // character that should be read
    size_t _l = _parse_symbol_dict.longest_match(_line.cbegin() + _i, _line.cend());
    if (_l == 0) { // normal character
        _word.push_back(_c);
        continue;
    }
        // special character
    _commands.build_word(_word); _word.clear();
    _relation = _line.substr(_i, _l); _i += _l;
    if (pipe_symbol(_relation)) {
        _commands.build_instruction();
        _commands.need_further_input();
        break;
    }
    else if (join_symbol(_relation)) {
        _commands.build_instruction();
        _commands.build_command(_relation);
        _commands.need_further_input();
        break;
    }
    else if (redirect_symbol(_relation)) {
        _commands.prepare_redirection(_relation);
        _commands.need_further_input();
        break;
    }
    else if (background_symbol(_relation)) {
        _relation.clear();
        _commands.build_instruction();
        _commands.build_command(_relation, true);
        break;
    }}}}
    return 0;
}
bool shell_backend::compile() {
    if (_parse_status != parse_status::eof) {
        clear(); return false;
    }
    for (auto& _cmd : _commands._parsed_command) {
        for (auto& _ins : _cmd._instructions) {
            for (auto& _w : _ins) {
                if (!compile_word(_w)) return false;
            }
        }
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
const std::string shell_backend::build_information() {
    std::string _info;
    _info.push_back('[');
    _info.append(_cwd.filename());
    size_t _bg_cnt = 0;
    for (const auto& [_p, _j] : _proc_map) {
        if (_j.background()) ++_bg_cnt;
    }
    _info.append(std::string(" (") + std::to_string(_bg_cnt) + ")");
    _info.push_back(']');
    return _info;
}
const std::string shell_backend::build_tab_next(const std::string& _line) {
    parse_tab(_line);
    trie_tree _tab_next_dict;
    if (tab_type_check(file, _word2bc_type)) {
        const auto& _file_tab_next = build_file_tab_next(_word_2bc);
        if (_file_tab_next.empty()) return "";
        _tab_next_dict.add(_file_tab_next);
    }
    if (tab_type_check(program, _word2bc_type)) {
        const auto& _program_tab_next = build_program_tab_next(_word_2bc);
        if (_program_tab_next.empty()) return "";
        _tab_next_dict.add(_program_tab_next);
    }
    if (tab_type_check(env, _word2bc_type)) {
        const auto& _env_tab_next = build_env_tab_next(_word_2bc);
        if (_env_tab_next.empty()) return "";
        _tab_next_dict.add(_env_tab_next);
    }
    return _tab_next_dict.next("");
}
const std::vector<std::string> shell_backend::build_tab_list(const std::string& _line) {
    parse_tab(_line);
    // tab classification
    std::vector<std::string> _tab_list;
    if (tab_type_check(file, _word2bc_type)) {
        const auto& _file_tab_list = build_file_tab_list(_word_2bc);
        _tab_list.insert(_tab_list.end(), _file_tab_list.cbegin(), _file_tab_list.cend());
    }
    if (tab_type_check(program, _word2bc_type)) {
        const auto& _program_tab_list = build_program_tab_list(_word_2bc);
        _tab_list.insert(_tab_list.end(), _program_tab_list.cbegin(), _program_tab_list.cend());
    }
    if (tab_type_check(env, _word2bc_type)) {
        const auto& _env_tab_list = build_env_tab_list(_word_2bc);
        _tab_list.insert(_tab_list.end(), _env_tab_list.cbegin(), _env_tab_list.cend());
    }
    return _tab_list;
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
    if (_s.empty()) return;
    _history.push_back(_s);
    _history_iterator = _history.cend();
}
void shell_backend::kill_all_process() {
    kill_process();
}


int shell_backend::execute_command(const command& _cmd) {
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
        const auto _r = execute_builtin_instruction(_main_ins);
        // re-redirect
        if (_input_file != nullptr) {
            fclose(_input_file);
            dup2(_old_in, _in);
        }
        if (_output_file != nullptr) {
            fclose(_output_file);
            dup2(_old_out, _in);
        }
        return _r;
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
                exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
        printf("[%ld](%d) \t%s\n", _task_serial_i, _pid, _main_ins.front().c_str());
        _j.set_serial(_task_serial_i);
        _proc_map.insert({_pid, _j});
        append_background_job(_pid);
    }
    return 0;
}
int shell_backend::execute_instruction(const std::vector<std::string>& _args) {
    if (_builtin_instruction_map.contains(_args[0].c_str())) { // for built-in instruction
        return execute_builtin_instruction(_args);
    }
    else { // for other instruction
        std::vector<const char*> _cmd_args;
        for (const auto& _arg : _args) {
            _cmd_args.emplace_back(_arg.c_str());
        }
        _cmd_args.emplace_back(nullptr);
        execvp(_args[0].c_str(), const_cast<char* const*>(_cmd_args.data()));
        return -1;
    }
}
int shell_backend::execute_builtin_instruction(const std::vector<std::string>& _args) {
    if (!builtin_instruction_check(_args[0], _args)) {
        printf("wrong parameter count\n");
        return -1;
    }
    (this->*_builtin_instruction_map.at(_args[0])._handler)(_args);
    return 0;
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
            output_debug("send SIGINT to %d\n", _j.pid());
            ::kill(-_j.pgid(), SIGINT);
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
void shell_backend::kill_process(pid_t _pid) {
    int _status;
    if (_pid == 0) {}
    else if (_pid < 0) {
        for (const auto& [_p, _j] : _proc_map) {
            output_debug("send SIGKILL to %d\n", _j.pid());
            ::kill(-_j.pid(), SIGKILL);
        }
        while ((_pid = waitpid(-1, &_status, WNOHANG)) > 0) {
            waitpid_handler(_pid, _status);
        }
    }
    else {
        output_debug("send SIGKILL to %d\n", _pid);
        ::kill(-_pid, SIGKILL);
        while ((_pid = waitpid(_pid, &_status, WSTOPPED)) == -1 && errno == EINTR);
        waitpid_handler(_pid, _status);
    }
}


bool shell_backend::builtin_instruction_check(const std::string& _cmd, const std::vector<std::string>& _args) {
    if (!is_builtin_instruction(_cmd)) return false;
    const auto& _details = _builtin_instruction_map.at(_cmd);
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
    return _builtin_instruction_map.contains(_cmd);
}
void shell_backend::_M_pwd(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    printf("%s\n", _cwd.c_str());
}
void shell_backend::_M_cd(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
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
void shell_backend::_M_history(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    for (const auto& _s : _history) {
        printf("%s\n", _s.c_str());
    }
}
void shell_backend::_M_quit(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    kill_process();
    exit(EXIT_FAILURE);
}
void shell_backend::_M_bg(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const size_t _s = std::stoi(_args[1]);
    if (!_bg_map.contains(_s)) {
        printf("wrong task id\n");
        return;
    }
    const pid_t _pid = _bg_map[_s];
    auto& _j = _proc_map.at(_pid);
    ::kill(-_j.pgid(), SIGCONT);
}
void shell_backend::_M_fg(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
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
void shell_backend::_M_jobs(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const std::vector<std::string> _states = {"Name", "State"};
    for (const auto& [_s, _p] : _bg_map) {
        const auto _vs = proc::get_status(_p, _states);
        if (!_vs.empty()) {
            printf("[%ld](%d) \t%s \t%s\n", _s, _p, _vs[0].c_str(), _vs[1].c_str());
        }
    }
}
void shell_backend::_M_kill(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const size_t _i = std::stoi(_args[1]);
    if (!_bg_map.contains(_i)) {
        printf("wrong task id\n");
        return;
    }
    pid_t _pid = _bg_map[_i];
    auto& _j = _proc_map.at(_pid);
    assert(_j.pgid() > 0);
    kill_process(_j.pgid());
}
void shell_backend::_M_sleep(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    const auto _x = std::stoi(_args[1]);
    std::this_thread::sleep_for(std::chrono::seconds(_x));
}
void shell_backend::_M_echo(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    for (size_t _i = 1; _i < _args.size();) {
        printf("%s", _args[_i].c_str());
        ++_i;
        if (_i != _args.size()) printf(" ");
    }
    printf("\n");
}
void shell_backend::_M_setenv(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    if (_args.size() == 2) {
        add_env_variable(_args[1], "");
    }
    else {
        add_env_variable(_args[1], _args[2]);
    }
}
void shell_backend::_M_unsetenv(const std::vector<std::string>& _args) {
    // BUILT_IN_INSTRUCTION_ARGS_CHECK(_args);
    for (size_t _i = 1; _i < _args.size(); ++_i) {
        del_env_variable(_args[_i]);
    }
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
bool shell_backend::pipe_symbol(const std::string& _r) const {
    return _r == "|";
}
bool shell_backend::join_symbol(const std::string& _r) const {
    return _r == "||" || _r == ";" || _r == "&&";
}
bool shell_backend::redirect_symbol(const std::string& _r) const {
    return _r == ">" || _r == ">>" || _r == "<" /*|| _r == "<<"*/;
}
bool shell_backend::background_symbol(const std::string& _r) const {
    return _r == "&";
}
bool shell_backend::compile_word(std::string& _s) const {
    // supposed that %_s meets grammer rule
    for (size_t _i = 0; _i < _s.size();) {
        const auto _c = _s[_i];
        if (_c == '\\') {
            _s.erase(_s.cbegin() + _i);
        }
        else if (_c == '\'') {
            _s.erase(_s.cbegin() + _i);
            while (_i != _s.size() && _s[_i] != '\'') ++_i;
            if (_i != _s.size()) { // _s[_i] == '\''
                _s.erase(_s.cbegin() + _i);
            }
        }
        // else if (_c == '\"') {}
        // else if (_c == '`') {}
        else if (_c == '$') {
            // expand env variable
            const size_t _l = env_variable_length(_s, _i + 1);
            if (_l != 0) {
                const auto& _ev = env_variable(_s.substr(_i + 1, _l));
                if (_ev.first) {
                    _s.erase(_s.cbegin() + _i, _s.cbegin() + _i + 1 + _l);
                    _s.insert(_i, _ev.second);
                    _i += _ev.second.size();
                    continue;
                }
            }
        }
        ++_i;
    }
    return true;
}


bool shell_backend::env_symbol(const std::string& _r) const {
    return _r == "$";
}
void shell_backend::parse_tab(const std::string& _line) {
    _word_2bc.clear(); _word2bc_type = tab_type::file | tab_type::program;
    if (_line.empty()) {
        _word2bc_type = tab_type::none; return;
    }
    auto _tab_parse_status = parse_status::parsing; size_t _i = 0; 
    // tab parse
    while (_tab_parse_status != parse_status::eof) {
        // skip blank between words
        while (_i != _line.size() && _line[_i] == ' ') { ++_i; }
        if (_i == _line.size()) break;
        // process one word in a cycle at least
        for (; _i != _line.size(); ++_i) {
            const auto& _c = _line[_i];
            if (_c == '\\') {
                ++_i;
                if (_i == _line.size()) {
                    _tab_parse_status = parse_status::eof; break;
                }
                _word_2bc.push_back(_line[_i]);
            }
            else if (_c == '\'') {
                ++_i;
                while (true) {
                    if (_i == _line.size()) {
                        _tab_parse_status = parse_status::eof; break;
                    }
                    if (_line[_i] == '\'') {
                        ++_i; break;
                    }
                    _word_2bc.push_back(_line[_i]); ++_i;
                }
            }
            // else if (_c == '\"') {}
            // else if (_c == '`') {}
            else if (_c == ' ') {
                _word_2bc.clear(); _word2bc_type = tab_type::file; break;
            }
            else if (_c == '$') {
                _word_2bc.clear(); _word2bc_type = tab_type::env;
            }
            else {
                size_t _l = _tab_symbol_dict.longest_match(_line.cbegin() + _i, _line.cend());
                if (_l == 0) { // normal character
                    _word_2bc.push_back(_c);
                    continue;
                }
                _word_2bc.clear(); _word2bc_type = tab_type::file;
                const std::string _symbol = _line.substr(_i, _l); _i += _l;
                if (pipe_symbol(_symbol)) {
                    _word2bc_type = tab_type::program;
                }
                else if (join_symbol(_symbol)) {
                    _word2bc_type = tab_type::program;
                }
                else if (redirect_symbol(_symbol)) {
                    _word2bc_type = tab_type::file;
                }
                else if (env_symbol(_symbol)) {
                    _word2bc_type = tab_type::env;
                }
            }
        }
    }
}
std::string shell_backend::build_program_tab_next(const std::string& _s) {
    return _program_dict.next(_s);
}
std::string shell_backend::build_file_tab_next(const std::string& _incompleted_path) {
    if (_incompleted_path.empty()) {
        fetch_cwd_dict();
        return _cwd_dict.next("");
    }
    const bool _end_with_separator = _incompleted_path.back() == '/';
    std::filesystem::path _path(_incompleted_path);
    const std::string _final_part = (_end_with_separator ? "" : _path.filename().string());
    if (!_end_with_separator) {
        _path = _path.parent_path();
    }
    if (_path.empty()) _path = _cwd;
    if (!std::filesystem::exists(_path)) {
        return {};
    }
    fetch_file_dict(_path);
    return _file_dict.next(_final_part);
}
std::string shell_backend::build_env_tab_next(const std::string& _s) {
    return _env_dict.next(_s);
}
std::vector<std::string> shell_backend::build_program_tab_list(const std::string& _s) {
    return _program_dict.tab(_s);
}
std::vector<std::string> shell_backend::build_file_tab_list(const std::string& _incompleted_path) {
    if (_incompleted_path.empty()) {
        fetch_cwd_dict();
        return _cwd_dict.tab("");
    }
    const bool _end_with_separator = _incompleted_path.back() == '/';
    std::filesystem::path _path(_incompleted_path);
    const std::string _final_part = (_end_with_separator ? "" : _path.filename().string());
    if (!_end_with_separator) {
        _path = _path.parent_path();
    }
    if (_path.empty()) _path = _cwd;
    if (!std::filesystem::exists(_path)) {
        return {};
    }
    fetch_file_dict(_path);
    return _file_dict.tab(_final_part);
}
std::vector<std::string> shell_backend::build_env_tab_list(const std::string& _s) {
    return _env_dict.tab(_s);
}
void shell_backend::fetch_program_dict() {
    const std::string _env_path = env_variable("PATH").second;
    size_t _l = 0; size_t _r = 0;
    asp::filesystem::file_handler _handler = [this](const std::filesystem::path& _i) {
        const auto _file_name = _i.filename().string();
        if (_file_name == "." || _file_name == "..") {}
        else { _program_dict.add(_file_name); }
    };
    while (_l != _env_path.size()) {
        while (_r != _env_path.size() && _env_path[_r] != ':') { ++_r; }
        const std::filesystem::path _path {_env_path.substr(_l, _r - _l)};
        asp::filesystem::for_each_file(_path, _handler);
        if (_r != _env_path.size()) ++_r;
        _l = _r;
    }
    for (const auto& [_k, _v] : _builtin_instruction_map) {
        _program_dict.add(_k);
    }
}
void shell_backend::fetch_file_dict(const std::filesystem::path& _path) {
    _file_dict.clear();
    asp::filesystem::file_handler _handler = [this](const std::filesystem::path& _i) {
        const auto _file_name = _i.filename().string();
        if (_file_name == "." || _file_name == "..") {}
        else { _file_dict.add(_file_name); }
    };
    asp::filesystem::for_each_file(_path, _handler);
}
void shell_backend::fetch_env_dict() { // just init
    _env_dict.clear();
    for (const auto& [_k, _v] : _preseted_env_map) {
        _env_dict.add(_k);
    }
    for (const auto& [_k, _v] : _customed_env_map) {
        _env_dict.add(_k);
    }
}
void shell_backend::fetch_cwd_dict() {
    _cwd_dict.clear();
    asp::filesystem::file_handler _handler = [this](const std::filesystem::path& _i) {
        const auto _file_name = _i.filename().string();
        if (_file_name == "." || _file_name == "..") {}
        else { _cwd_dict.add(_file_name); }
    };
    asp::filesystem::for_each_file(_cwd, _handler);
}


void shell_backend::init_env_variable_map() {
    _preseted_env_map.clear(); _customed_env_map.clear();
    for (size_t _i = 0; environ[_i] != nullptr; ++_i) {
        const std::string _s = environ[_i];
        size_t _j = _s.find_first_of('=');
        _preseted_env_map[_s.substr(0, _j)] = _s.substr(_j + 1);
    }
    // initialize %_customed_env_map
}
void shell_backend::add_env_variable(const std::string& _k, const std::string& _v) {
    _customed_env_map[_k] = _v;
    _env_dict.add(_k);
}
void shell_backend::del_env_variable(const std::string& _k) {
    _preseted_env_map.erase(_k);
    _customed_env_map.erase(_k);
    _env_dict.del(_k);
}
size_t shell_backend::env_variable_length(const std::string& _s, size_t _i) const {
    for (size_t _j = _i; _j < _s.size(); ++_j) {
        if (!isdigit(_s[_j]) && !isalpha(_s[_j])) {
        // if (_s[_j] == ' ') {
            return _j - _i;
        }
    }
    return _s.size() - _i;
}
std::pair<bool, const std::string&> shell_backend::env_variable(const std::string& _k) const {
    if (_customed_env_map.contains(_k)) {
        return {true, _customed_env_map.at(_k)};
    }
    if (_preseted_env_map.contains(_k)) {
        return {true, _preseted_env_map.at(_k)};
    }
    return {false, _empty_string};
}


void shell_backend::load_history() {
    // todo
    _history_iterator = _history.cend();
}
};