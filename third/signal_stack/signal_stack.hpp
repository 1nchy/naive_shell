#ifndef _ASP_SIGNAL_STACK_HPP_
#define _ASP_SIGNAL_STACK_HPP_

#include <csignal>
#include <functional>
#include <vector>
#include <unordered_map>

namespace asp {

class signal_stack;

class signal_stack {
public:
    signal_stack() = default;
    signal_stack(const signal_stack&) = delete;
    signal_stack& operator=(const signal_stack&) = delete;
    ~signal_stack() = default;
    typedef void(*handler_t)(int);
    // 
    size_t size() const { return _data.size(); }
    // whether the signal handler is set
    bool empty(unsigned) const;
    // set signal handler
    bool build(unsigned, handler_t);
    // set signal handler with flag
    bool build(unsigned, int, handler_t);
    // set signal handler with flag and mask
    bool build(unsigned, int, handler_t, std::initializer_list<int>);
    // set signal handler
    bool build(unsigned, struct sigaction);
    // restore previous signal handler
    bool restore(unsigned);
    // set initial handler
    bool reset(unsigned);
    // clear all handler of certain signal in stack
    bool clear(unsigned);
private:
/**
 * When _data[_i].size() == 1, the only element must be initial signal handler.
 * Thus, when building handler, if _data[_i].empty(), we need to store initial signal handler,
 * which is used to restore handler.
*/
    std::unordered_map<unsigned, std::vector<struct sigaction>> _data;
};
};

#endif // _ASP_SIGNAL_STACK_HPP_