#ifndef _ASP_SHELL_HPP_
#define _ASP_SHELL_HPP_

#include <string>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

#include <iostream>

#include "command.hpp"

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

    void execute_command(const command&);
    void execute_instruction(const std::vector<std::string>&);
    void execute_builtin_instruction(const std::vector<std::string>&);

    std::string build_information();
private:
    // typedef void(shell::*internal_instruction_handler)(void);
    using internal_instruction_handler = void(shell::*)(const std::vector<std::string>&);
    // using internal_instruction_handler = void(void);
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
    void job(const std::vector<std::string>&);
    void echo(const std::vector<std::string>&);

private:
    shell_editor& _editor;
    // std::vector<command> _parsed_command;
    command_sequence _commands;
    // std::vector<std::string> _command_relation;
    std::unordered_set<pid_t> _child_processes;
    std::unordered_set<pid_t> _current_processes;

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
        {"pwd", {&shell::pwd}}, {"cd", {&shell::cd, 1, 0}},
        {"history", {&shell::history}}, {"quit", {&shell::quit}},
        {"bg", {&shell::bg}}, {"job", {&shell::job}},
        {"echo", {&shell::echo, 1, 0}}
    };
};

};

#endif // _ASP_SHELL_HPP_