#include <exception>
#include <iostream>

void CheckConnectionErrors();
void CheckQueues();

int main() {
    try {
        CheckConnectionErrors();
        CheckQueues();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
