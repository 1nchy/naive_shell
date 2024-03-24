#include "frontend.hpp"

int main(void) {
    asp::shell_frontend::singleton().run();
    return 0;
}