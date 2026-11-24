#include "../file/Tests.hpp"

#include <iostream>

void CheckStatusArray();
void CheckStatusEntry();

int main() {
    try {
        CheckStatusArray();
        CheckStatusEntry();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
