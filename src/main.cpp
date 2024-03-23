#include "editor.hpp"
#include "shell.hpp"

int main(void) {
    auto& shell_singleton = asp::shell::singleton();
    while (true) {
        if (!shell_singleton.wait()) break;
        if (!shell_singleton.compile()) continue;
        shell_singleton.execute();
    }
    return 0;
}