#ifndef _ICY_OUTPUT_HPP_
#define _ICY_OUTPUT_HPP_

#include <sstream>
#include <utility>
#include <unistd.h>
#include <tuple>

namespace icy {
namespace output {

enum output_level {
    all = 0,
    debug,
    info,
    warn,
    error,
    fatal
};

#define output_debug(...) output_func(output::debug, __FILE__, __LINE__, __VA_ARGS__)
#define output_info(...) output_func(output::info, __FILE__, __LINE__, __VA_ARGS__)
#define output_warn(...) output_func(output::warn, __FILE__, __LINE__, __VA_ARGS__)
#define output_error(...) output_func(output::error, __FILE__, __LINE__, __VA_ARGS__)
#define output_fatal(...) output_func(output::fatal, __FILE__, __LINE__, __VA_ARGS__)

void set_output_level(output_level _l);

void output_func(output_level _l, const char* _file, int _line, const char* _fmt, ...);

namespace {
inline int _S_iprintf(std::stringstream& _ss, int _fd, const char* _fmt) {
    for (const char* _p = _fmt; *_p != '\0'; ++_p) {
        _ss << *_p;
    }
    return 0;
};
template <typename _Tp, typename... _Args> int _S_iprintf(std::stringstream& _ss, int _fd, const char* _fmt, _Tp&& _arg, _Args&&... _args) {
    ++_fmt;
    _ss << _arg;
    for (const char* _p = _fmt; *_p != '\0'; ++_p) {
        if (*_p == '%') {
            if (*(_p+1) != '%') {
                return _S_iprintf(_ss, _fd, _p, std::forward<_Args>(_args)...) + 1;
            }
            ++_p;
        }
        _ss << *_p;
    }
    return 1;
};
}

template <typename... _Args> int iprintf(int _fd, const char* _fmt, _Args&&... _args) {
    if (_fmt == nullptr) return -1;
    std::stringstream _ss;
    int _ret = 0;
    for (const char* _p = _fmt; *_p != '\0'; ++_p) {
        if (*_p == '%') {
            if (*(_p+1) != '%') {
                _ret = _S_iprintf(_ss, _fd, _p, std::forward<_Args>(_args)...);
                break;
            }
            ++_p;
        }
        _ss << *_p;
    }
    const std::string& _str = _ss.str();
    std::ignore = write(_fd, _str.c_str(), _str.size());
    return 0;
};

}
}

#endif // _ICY_OUTPUT_HPP_