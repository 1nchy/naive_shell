#ifndef _ASP_VIRTUAL_EDITOR_HPP_
#define _ASP_VIRTUAL_EDITOR_HPP_

#include <iostream>
#include <string>

namespace asp {
class shell_editor;
class shell_editor {
public:
    shell_editor() {}
    virtual ~shell_editor() {}
    // virtual std::istream& in() = 0;
    virtual int in() const = 0;
    // virtual std::ostream& out() = 0;
    virtual int out() const = 0;
    virtual void show_information(const std::string& _s) = 0;
    virtual void show_prompt() = 0;
    virtual const std::string& line() = 0;
};
};

#endif // _ASP_VIRTUAL_EDITOR_HPP_