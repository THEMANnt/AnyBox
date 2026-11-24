#include "Fixture.hpp"
#include <anybox/ExceptionTest.hpp>
#include <anybox/vulkan/Context.hpp>

int main() {
    return AnyBox::Tests::Run([] {
        AnyBox::Vulkan::Context context(0);
        StructuredTests::CheckBranches(context);
        StructuredTests::CheckLoop(context);
        StructuredTests::CheckTyped(context);
        StructuredTests::CheckExits(context);
        StructuredTests::CheckClones(context);
        StructuredTests::CheckFloatLoop(context);
    });
}
