#ifndef _ASP_COMMAND_HPP_
#define _ASP_COMMAND_HPP_

#include <vector>
#include <string>

#include <cassert>

namespace asp {

struct command;
struct command_sequence;

struct command {
    std::vector<std::vector<std::string>> _instructions;
    std::string _redirect_in;
    std::string _redirect_out;
    short _redirect_out_type = 0; // 0 none, 1 create, 2 append
    bool _background = false;

    const std::vector<std::string>& main_ins() const {
        if (_instructions.empty()) exit(EXIT_FAILURE);
        return _instructions.back();
    }
    void append_word(const std::string& _s) {
        if (_instructions.empty()) exit(EXIT_FAILURE);
        _instructions.back().emplace_back(_s);
    }
    void create_instruction() {
        _instructions.emplace_back();
    }
    size_t size() const { return _instructions.size(); }
};

struct command_sequence {
    std::vector<command> _parsed_command;
    std::vector<std::string> _command_relation;

    void build_word(const std::string& _s);
    void build_instruction();
    void build_command(const std::string& _relation, bool _bg = false);
    void prepare_redirection(const std::string& _redir);
    size_t size() const { return _parsed_command.size(); }
    void ready_in() { _in_ready = true; }
    void ready_out() { _out_ready = true; }
    bool further_input() const { return _further_input; }
    void need_further_input() { _further_input = true; }
    void clear();
private:
    bool _in_ready = false; bool _out_ready = false;
    bool _new_ins = true; bool _new_cmd = true;
    bool _further_input = false;
};

};

#endif // _ASP_COMMAND_HPP_