#include "editor.hpp"
#include "shell.hpp"

int main(void) {
    asp::editor _editor;
    asp::shell _shell(_editor);
    while (true) {
        if (!_shell.wait()) break;
        if (!_shell.compile()) continue;
        _shell.execute();
    }
    return 0;
}