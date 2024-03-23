#include "command.hpp"

namespace asp {
void command_sequence::build_word(const std::string& _s) {
    if (_s.empty()) return;
    _further_input = false;
    if (_in_ready) {
        _parsed_command.back()._redirect_in = _s;
        _in_ready = false;
    }
    else if (_out_ready) {
        _parsed_command.back()._redirect_out = _s;
        _out_ready = false;
    }
    else {
        if (_new_cmd) {
            _parsed_command.emplace_back();
        }
        if (_new_ins) {
            _parsed_command.back().create_instruction();
        }
        _parsed_command.back().append_word(_s);
        _new_cmd = false; _new_ins = false;
    }
}
void command_sequence::build_instruction() {
    if (_new_ins) {
        return;
    }
    _new_ins = true;
}
void command_sequence::build_command(const std::string& _relation, bool _bg) {
    if (_new_cmd) {
        return;
    }
    _parsed_command.back()._background = _bg;
    _command_relation.emplace_back(_relation);
    _new_cmd = true; _new_ins = true;
}
void command_sequence::prepare_redirection(const std::string& _redir) {
    if (_redir == "<") {
        _in_ready = true;
    }
    else if (_redir == ">") {
        _out_ready = true;
        _parsed_command.back()._redirect_out_type = 1;
    }
    else if (_redir == ">>") {
        _out_ready = true;
        _parsed_command.back()._redirect_out_type = 2;
    }
}
void command_sequence::clear() {
    _parsed_command.clear();
    _command_relation.clear();
    _in_ready = false; _out_ready = false;
    _new_cmd = true; _new_ins = true;
    _further_input = false;
}
};