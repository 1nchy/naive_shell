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
    const std::string build_information() override;
    const std::string build_tab_next(const std::string&) override;
    const std::vector<std::string> build_tab_list(const std::string&) override;
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
    void _M_pwd(const std::vector<std::string>&);
    void _M_cd(const std::vector<std::string>&);
    void _M_history(const std::vector<std::string>&);
    void _M_quit(const std::vector<std::string>&);
    void _M_bg(const std::vector<std::string>&);
    void _M_fg(const std::vector<std::string>&);
    void _M_jobs(const std::vector<std::string>&);
    void _M_kill(const std::vector<std::string>&);
    void _M_sleep(const std::vector<std::string>&);
    void _M_echo(const std::vector<std::string>&);
    void _M_setenv(const std::vector<std::string>&);
    void _M_unsetenv(const std::vector<std::string>&);
    const std::unordered_map<std::string, builtin_instruction> _builtin_instruction = {
        {"pwd", {&shell_backend::_M_pwd}}, {"cd", {&shell_backend::_M_cd, 1, 2}},
        {"history", {&shell_backend::_M_history}}, {"quit", {&shell_backend::_M_quit}},
        {"bg", {&shell_backend::_M_bg, 2, 2}}, {"fg", {&shell_backend::_M_fg, 2, 2}},
        {"jobs", {&shell_backend::_M_jobs}}, {"kill", {&shell_backend::_M_kill, 2, 2}},
        {"sleep", {&shell_backend::_M_sleep, 2, 2}},
        {"echo", {&shell_backend::_M_echo, 1, 0}},
        {"setenv", {&shell_backend::_M_setenv, 2, 3}},
        {"unsetenv", {&shell_backend::_M_unsetenv, 2, 2}},
    };
private: // command parsing
    enum parse_status {
        parsing,
        eof,
        backslash,
        single_quote,
        double_quotes,
        back_quote,
    };
    bool end_of_line(parse_status _s = parse_status::parsing);
    // |
    bool pipe_symbol(const std::string&) const;
    // || ; &&
    bool join_symbol(const std::string&) const;
    // > >> <
    bool redirect_symbol(const std::string&) const;
    bool background_symbol(const std::string&) const;
    bool compile_word(std::string&) const;
    parse_status _parse_status = parse_status::parsing;
    const trie_tree _parse_symbol_dict = {
        ">", ">>", "<", /*"<<",*/ ";", "|", "||", "&", "&&",
    };
    std::string _word;
    std::string _relation;
private: // tab
    enum tab_type : short { // bitmap
        none = 0x0,
        file = 0x1 << 0,
        program = 0x1 << 1,
        env = 0x1 << 2,
        // cwd = 0x1 << 3,
    };
    bool env_symbol(const std::string&) const;
    #define tab_type_check(_type, _t) ((tab_type::_type) & (_t))
    const trie_tree _tab_symbol_dict = {
        ">", ">>", "<", /*"<<",*/ ";", "|", "||", "&", "&&", "$",
    };
    std::string _word_2bc;
    tab_type _word2bc_type = tab_type::file;
    trie_tree _program_dict;
    trie_tree _file_dict;
    trie_tree _cwd_dict;
    trie_tree _env_dict;
    void parse_tab(const std::string& _line);
    std::string build_program_tab_next(const std::string&);
    std::string build_file_tab_next(const std::string&);
    std::string build_env_tab_next(const std::string&);
    std::vector<std::string> build_program_tab_list(const std::string&);
    std::vector<std::string> build_file_tab_list(const std::string&);
    std::vector<std::string> build_env_tab_list(const std::string&);
    void fetch_program_dict();
    void fetch_file_dict(const std::filesystem::path&);
    void fetch_env_dict();
    void fetch_cwd_dict();
private: // environment variables
    std::unordered_map<std::string, std::string> _preseted_env_map;
    std::unordered_map<std::string, std::string> _customed_env_map;
    // ~~trie_tree _env_dict~~;
    void init_env_variable_map();
    void add_env_variable(const std::string&, const std::string&);
    void del_env_variable(const std::string&);
    size_t env_variable_length(const std::string&, size_t) const;
    std::pair<bool, const std::string&> env_variable(const std::string&) const;
private: // history
    void load_history();
    std::vector<std::string> _history;
    typename decltype(_history)::const_iterator _history_iterator;
private: // process
    command_sequence _commands;
    std::unordered_map<pid_t, job> _proc_map;
    std::map<size_t, pid_t> _bg_map;
    size_t _task_serial_i = 1;
private: // path
    std::filesystem::path _cwd;
    std::filesystem::path _prev_cwd;
    std::filesystem::path _home_dir;
};

};

#endif // _ASP_BACKEND_HPP_