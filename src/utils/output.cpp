#include "utils/output.hpp"

#include <cstdarg>

#include <cstdio>

namespace icy {
namespace output {

static output_level _output_level = output_level::all;

void set_output_level(output_level _l) {
    _output_level = _l;
}

void output_func(output_level _l, const char* _file, int _line, const char* _fmt, ...) {
    if (_l < _output_level) return;
    va_list _args;
    va_start(_args, _fmt);
    printf("[%s](%d) ", _file, _line);
    vprintf(_fmt, _args);
    va_end(_args);
}
}
};