#include "DxilModule.hpp"
#include "Fixtures.hpp"
#include <anybox/ExceptionTest.hpp>
#include <anybox/graphics/container/View.hpp>
#include <function.hpp>
#include <instruction.hpp>
#include <cast.hpp>
#include <array>
#include <fstream>
#include <future>

namespace {
std::vector<std::byte> MakeContainer() {
    std::ifstream input(ANYBOX_DXIL_FIXTURE, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open upstream DXIL test fixture");
    auto length = input.tellg();
    if (length <= 0 || length > 65536) throw std::runtime_error("Invalid upstream DXIL test fixture size");
    auto size = static_cast<std::size_t>(length);
    std::vector<std::byte> program((24 + size + 3) & ~std::size_t{3});
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(program.data() + 24), length)) throw std::runtime_error("Cannot read upstream DXIL test fixture");

    Fixtures::Set(program, 0, 0x50060);
    Fixtures::Set(program, 4, static_cast<std::uint32_t>(program.size() / 4));
    Fixtures::Set(program, 8, 0x4C495844);
    Fixtures::Set(program, 12, 0x100);
    Fixtures::Set(program, 16, 16);
    Fixtures::Set(program, 20, static_cast<std::uint32_t>(size));
    return AnyBox::Graphics::Container::BuildDxbc(std::array<AnyBox::Graphics::Container::Part, 1>{{{0x4C495844, program}}});
}

void CheckModule(std::span<const std::byte> code) {
    AnyBox::Shader::DxilModule module(code);
    std::size_t blocks = 0;
    std::size_t instructions = 0;
    std::size_t calls = 0;
    for (auto* function : module.GetModule()) {
        for (auto& block : *function) {
            ++blocks;
            Fixtures::Require(block.getTerminator() != nullptr);
            for (auto& instruction : block) {
                ++instructions;
                if (LLVMBC::isa<LLVMBC::CallInst>(&instruction)) ++calls;
            }
        }
    }
    if (blocks == 0 || instructions <= blocks || calls == 0) throw std::runtime_error("DXIL test fixture has unexpected block/instruction/call counts: " + std::to_string(blocks) + "/" + std::to_string(instructions) + "/" + std::to_string(calls));
    Fixtures::Reject<std::logic_error>([&] { AnyBox::Shader::DxilModule nested(code); });
}
}

int main() {
    return AnyBox::Tests::Run([] {
        auto code = MakeContainer();
        CheckModule(code);
        {
            AnyBox::Shader::DxilModule outer(code);
            auto worker = std::async(std::launch::async, [&] { CheckModule(code); });
            worker.get();
        }
        Fixtures::Reject<std::invalid_argument>([] { AnyBox::Shader::DxilModule invalid({}); });
        CheckModule(code);
    });
}
