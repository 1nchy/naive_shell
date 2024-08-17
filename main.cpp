#include "frontend.hpp"

int main(void) {
    icy::shell_frontend::singleton().activate();
    icy::shell_frontend::singleton().deactivate();
    return 0;
}