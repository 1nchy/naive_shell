#include "frontend.hpp"

int main(void) {
    asp::shell_frontend::singleton().activate();
    asp::shell_frontend::singleton().deactivate();
    return 0;
}