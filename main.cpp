#include "editor.hpp"
#include "shell.hpp"

int main(void) {
    asp::editor _editor;
    asp::shell _shell(_editor);
    while (true) {
        if (!_shell.wait()) break;
        _shell.compile();
        // _shell.execute();
    }
    return 0;
}