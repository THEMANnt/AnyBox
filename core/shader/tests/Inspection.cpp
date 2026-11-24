#include <anybox/shader/Inspection.hpp>
#include <anybox/graphics/container/View.hpp>
#include <anybox/ExceptionTest.hpp>
#include "Fixtures.hpp"
#include <array>

int main() {
    return AnyBox::Tests::Run([] {
        using namespace Fixtures;
        using namespace AnyBox::Graphics::Container;
        Reject<std::invalid_argument>([] { AnyBox::Shader::InspectDxil({}); });
        auto empty = BuildDxbc({});
        Reject<std::invalid_argument>([&] { AnyBox::Shader::InspectDxil(empty); });
        auto invalid = MakeProgram();
        auto container = BuildDxbc(std::array<Part, 1>{{{0x4C495844, invalid}}});
        Reject<std::runtime_error>([&] { AnyBox::Shader::InspectDxil(container); });
        Reject<std::runtime_error>([&] { AnyBox::Shader::InspectDxil(container); });
    });
}
