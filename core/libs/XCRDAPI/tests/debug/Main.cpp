#include "Tests.hpp"

#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string_view>

int main(int argumentCount, char** arguments) {
    try {
        if (argumentCount == 3 && std::string_view(arguments[1]) == "--child") {
            unsigned flags = 0;
            std::string_view value(arguments[2]);
            auto result = std::from_chars(value.data(), value.data() + value.size(), flags);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || flags > 3) throw std::invalid_argument("Invalid debug test mode");
            AnyBox::Tests::CheckDebugChild(flags);
        } else {
            for (unsigned flags = 0; flags != 4; ++flags) AnyBox::Tests::CheckDebugSession(flags);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
