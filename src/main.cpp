// #include "editor.hpp"
// #include "shell.hpp"

#include "frontend.hpp"

int main(void) {
    auto& _frontend = asp::shell_frontend::singleton();
    _frontend.run();
    return 0;
}