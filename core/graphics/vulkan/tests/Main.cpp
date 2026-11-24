#include "Tests.hpp"
#include <iostream>

int main() {
    try {
        using namespace AnyBox::Vulkan;
        using namespace AnyBox::Vulkan::Tests;
        Context context(0);
        std::cout << "Vulkan adapter: " << context.GetProperties().deviceName << "; host pointer alignment " << context.GetHostPointerAlignment() << '\n';
        Storage storage;
        auto frames = storage.GetFrames();
        CheckLayout(context, storage, frames);
        std::array<std::uint64_t, 3> reversed{frames[2], frames[1], frames[0]};
        CheckLayout(context, storage, reversed);
        std::array<std::uint64_t, 3> repeated{frames[0], frames[0], frames[0]};
        CheckLayout(context, storage, repeated);
        std::array<std::uint64_t, 3> separate{frames[0], storage.GetExtraFrame(), frames[2]};
        CheckLayout(context, storage, separate);
        CheckImages(context);
        CheckImageViews(context);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
