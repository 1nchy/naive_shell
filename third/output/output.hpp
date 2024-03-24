#ifndef _ASP_OUTPUT_HPP_
#define _ASP_OUTPUT_HPP_

namespace asp {
namespace output {

enum output_level {
    debug,
    info,
    warn,
};

#define output_debug(...) output_func(output::debug, __FILE__, __LINE__, __VA_ARGS__)
#define output_info(...) output_func(output::info, __FILE__, __LINE__, __VA_ARGS__)

void set_output_level(output_level _l);

void output_func(output_level _l, const char* _file, int _line, const char* _fmt, ...);

}
}

#endif // _ASP_OUTPUT_HPP_