#ifndef _ICY_VIRTUAL_FRONTEND_HPP_
#define _ICY_VIRTUAL_FRONTEND_HPP_

#include <string>
#include <vector>

#include <unistd.h>

#include "virtual_backend.hpp"

namespace icy {

class frontend_interface;

class frontend_interface {
public:
    frontend_interface(int _in = STDIN_FILENO, int _out = STDOUT_FILENO, int _err = STDERR_FILENO)
     : _in(_in), _out(_out), _err(_err) {}
    frontend_interface(const frontend_interface&) = delete;
    frontend_interface& operator=(const frontend_interface&) = delete;
    virtual ~frontend_interface() {}
    virtual void show_information(const std::string& _s) = 0;
    virtual void show_prompt() = 0;
    virtual void activate() = 0;
    virtual void deactivate() = 0;
protected:
    virtual bool wait() = 0; // process one command sequence
protected:
    const int _in;
    const int _out;
    const int _err;
    std::string _command_line;
    backend_interface* _backend = nullptr;
};

};

#endif // _ICY_VIRTUAL_FRONTEND_HPP_