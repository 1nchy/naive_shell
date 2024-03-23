#include "editor.hpp"
#include "shell.hpp"

using asp::shell_singleton;

int main(void) {
    while (true) {
        if (!shell_singleton.wait()) break;
        if (!shell_singleton.compile()) continue;
        shell_singleton.execute();
    }
    return 0;
}