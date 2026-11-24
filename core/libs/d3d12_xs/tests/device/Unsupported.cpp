#include "Tests.hpp"
#include <string>

void AnyBox::Graphics::Tests::CheckUnsupportedDeviceMethods(Device* device) {
    for (auto slot : {128u, 134u, 255u}) {
        auto unavailable = Method<void (*)(void*, ...)>(device, slot);
        auto expected = std::string(ANYBOX_LIBRARY_NAME) + "!Device method " + std::to_string(slot) + " is not implemented";
        bool matched = false;
        Reject<std::runtime_error>([&] {
            try { unavailable(device, nullptr, nullptr, nullptr); }
            catch (const std::runtime_error& error) { matched = error.what() == expected; throw; }
        });
        Require(matched, "Unsupported Xbox device slot dispatched a different object's method");
    }
}
