#ifndef _ASP_BACKEND_HPP_
#define _ASP_BACKEND_HPP_

#include "virtual_backend.hpp"

#include <map>
#include <unordered_map>
#include "trie_tree/trie_tree.hpp"

#include "command.hpp"
#include "job.hpp"

namespace asp {

class shell_backend;

class shell_backend : public backend_interface {
private:
    shell_backend(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
public:
    static shell_backend& singleton(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO);
    virtual ~shell_backend();
    /**
     * @brief parse command line
     * @return -1 error, 0 normal, 1 need futher input
    */
    int parse(const std::string&) override;
    bool compile() override;
    bool execute() override;
    std::string build_information() override;
    std::vector<std::string> build_tab_list(const std::string&) override;
    const std::string& prev_history() override;
    const std::string& next_history() override;
    void append_history(const std::string&) override;
private:
    void execute_command(const command&) override;
    void execute_instruction(const std::vector<std::string>&) override;
    void execute_builtin_instruction(const std::vector<std::string>&) override;
public: // signal
    void sigchld_handler(int);
    void sigtstp_handler(int);
    void sigint_handler(int);
    void sigpipe_handler(int);
    void sigttin_handler(int);
    void sigttou_handler(int);
private: // process controller
    void clear();
    void waitpid_handler(pid_t _pid, int _status);
    size_t append_background_job(pid_t _pid);
    pid_t remove_background_job(size_t _i);
private: // built-in instruction
    // typedef void(shell_backend::*builtin_instruction_handler)(void);
    using builtin_instruction_handler = void(shell_backend::*)(const std::vector<std::string>&);
    struct builtin_instruction {
        builtin_instruction_handler _handler;
        size_t _min_args = 1;
        size_t _max_args = 1;
    };
    bool builtin_instruction_check(const std::string&, const std::vector<std::string>&);
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
    const std::unordered_map<std::string, builtin_instruction> _builtin_instruction = {
        {"pwd", {&shell_backend::pwd}}, {"cd", {&shell_backend::cd, 1, 2}},
        {"history", {&shell_backend::history}}, {"quit", {&shell_backend::quit}},
        {"bg", {&shell_backend::bg, 2, 2}}, {"fg", {&shell_backend::fg, 2, 2}},
        {"jobs", {&shell_backend::jobs}}, {"kill", {&shell_backend::kill, 2, 2}},
        {"echo", {&shell_backend::echo, 1, 0}},
        {"sleep", {&shell_backend::sleep, 2, 2}},
    };
private: // command parsing
    enum parse_status {
        parsing,
        eof,
        backslash,
        single_quote,
        double_quotes,
    };
    bool end_of_line(parse_status _s = parse_status::parsing);
    parse_status _parse_status = parse_status::parsing;
    std::string _word;
    std::string _relation;
    // bool _line_parsed = false;
private: // history
    void load_history();
    std::vector<std::string> _history;
    typename decltype(_history)::const_iterator _history_iterator;
private: // process
    command_sequence _commands;
    std::unordered_map<pid_t, job> _proc_map;
    std::map<size_t, pid_t> _bg_map;
    size_t _task_serial_i = 1;

    std::filesystem::path _cwd;
    std::filesystem::path _prev_cwd;
    std::filesystem::path _home_dir;

    trie_tree _cmd_symbol_dict;
    trie_tree _cmd_dict;
    trie_tree _path_dict;
};

};

#endif // _ASP_BACKEND_HPP_