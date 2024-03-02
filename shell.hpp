#ifndef _ASP_SHELL_HPP_
#define _ASP_SHELL_HPP_

#include <string>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

#include "virtual_editor.hpp"

#include "trie_tree.hpp"

namespace asp {
class shell;

class shell {
public:
    shell(shell_editor&);
    ~shell();
    bool wait();
    bool compile();
    bool execute();
private:
    // bool parse(const std::string& _s);
    void reset();
    size_t combine_instruction(size_t);
    void execute_combine_instruction(size_t, size_t);
    void execute_instruction(size_t);
    void execute_instruction_bg(size_t);
private:
    // typedef void(shell::*internal_instruction_handler)(void);
    using internal_instruction_handler = void(shell::*)(const std::vector<std::string>&);
    // using internal_instruction_handler = void(void);
    struct internal_instruction {
        internal_instruction_handler _handler;
        size_t _min_args = 1;
        size_t _max_args = 1;
    };

    void cwd(const std::vector<std::string>&);
    void cd(const std::vector<std::string>&);
    void history(const std::vector<std::string>&);
    void quit(const std::vector<std::string>&);
    void bg(const std::vector<std::string>&);
    void job(const std::vector<std::string>&);
    void echo(const std::vector<std::string>&);

private:
    shell_editor& _editor;
    // std::istream& _is; // for editor
    // std::ostream& _os; // for editor
    std::vector<std::vector<std::string>> _parsed_command;
    std::vector<std::pair<std::string, std::string>> _instruction_redirection;
    std::vector<short> _instruction_redirection_type; // 0 none, 1 create, 2 append
    std::vector<std::string> _instruction_relation;
    std::vector<bool> _background_instruction;
    std::unordered_set<pid_t> _child_processes;
    std::unordered_set<pid_t> _current_processes;

    // std::filesystem::path _cwd;
    // std::filesystem::path _home_dir;

    trie_tree _cmd_symbol_dict;
    // trie_tree _cmd_dict;
    // trie_tree _path_dict;

    // const std::unordered_set<std::string> _cmd_symbol_map;
    // const std::unordered_map<std::string, std::function<void()>> _cmd_map;
    // const std::unordered_set<std::string> _path_map;

    const std::unordered_map<std::string, internal_instruction> _internal_instruction = {
        {"cwd", {&shell::cwd}}, {"cd", {&shell::cd}},
        {"history", {&shell::history}}, {"quit", {&shell::quit}},
        {"bg", {&shell::bg}}, {"job", {&shell::job}},
        {"echo", {&shell::echo, 1, 0}}
    };
};

};

#endif // _ASP_SHELL_HPP_