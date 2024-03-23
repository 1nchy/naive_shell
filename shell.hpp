#ifndef _ASP_SHELL_HPP_
#define _ASP_SHELL_HPP_

#include <string>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

#include "command.hpp"
#include "job.hpp"

#include "virtual_editor.hpp"

#include "trie_tree.hpp"

namespace asp {
class shell;
extern shell shell_singleton;

void shell_sigchld_handler(int _sig);
void shell_sigtstp_handler(int _sig);
void shell_sigint_handler(int _sig);

class shell {
public:
    shell(shell_editor&);
    ~shell();
    bool wait();
    bool compile();
    bool execute();
public: // signal
    void sigchld_handler(int);
    void sigtstp_handler(int);
    void sigint_handler(int);
    void sigpipe_handler(int);
    void sigttin_handler(int);
    void sigttou_handler(int);

private:
    void line();
    // bool parse(const std::string& _s);
    void reset();
    size_t combine_instruction(size_t);

    void execute_command(const command&);
    void execute_instruction(const std::vector<std::string>&);
    void execute_builtin_instruction(const std::vector<std::string>&);

    std::string build_information();
private: // about built-in instruction
    // typedef void(shell::*internal_instruction_handler)(void);
    using internal_instruction_handler = void(shell::*)(const std::vector<std::string>&);
    struct internal_instruction {
        internal_instruction_handler _handler;
        size_t _min_args = 1;
        size_t _max_args = 1;
    };

    bool internal_instruction_check(const std::string&, const std::vector<std::string>&);
    bool is_builtin_instruction(const std::string&) const;

    void pwd(const std::vector<std::string>&);
    void cd(const std::vector<std::string>&);
    void history(const std::vector<std::string>&);
    void quit(const std::vector<std::string>&);
    void bg(const std::vector<std::string>&);
    void fg(const std::vector<std::string>&);
    void jobs(const std::vector<std::string>&);
    void kill(const std::vector<std::string>&);
    void echo(const std::vector<std::string>&);
    void sleep(const std::vector<std::string>&);
private:
    size_t search_in_background(pid_t _pid);
    void waitpid_handler(pid_t _pid, int _status);

private:
    std::string _line;
    shell_editor& _editor;
    command_sequence _commands;
    // std::unordered_set<pid_t> _fg_proc;
    // std::unordered_map<size_t, pid_t> _bg_proc;
    std::unordered_map<pid_t, job> _proc_map;
    std::unordered_map<size_t, pid_t> _bg_map;
    size_t _task_serial_i = 1;

    std::filesystem::path _cwd;
    std::filesystem::path _prev_cwd;
    std::filesystem::path _home_dir;

    trie_tree _cmd_symbol_dict;
    // trie_tree _cmd_dict;
    // trie_tree _path_dict;

    // const std::unordered_set<std::string> _cmd_symbol_map;
    // const std::unordered_map<std::string, std::function<void()>> _cmd_map;
    // const std::unordered_set<std::string> _path_map;

    const std::unordered_map<std::string, internal_instruction> _builtin_instruction = {
        {"pwd", {&shell::pwd}}, {"cd", {&shell::cd, 1, 2}},
        {"history", {&shell::history}}, {"quit", {&shell::quit}},
        {"bg", {&shell::bg, 2, 2}}, {"fg", {&shell::fg, 2, 2}},
        {"jobs", {&shell::jobs}}, {"kill", {&shell::kill, 2, 2}},
        {"echo", {&shell::echo, 1, 0}},
        {"sleep", {&shell::sleep, 2, 2}},
    };
};

};

#endif // _ASP_SHELL_HPP_