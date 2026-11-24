#include "DxilModule.hpp"
#include <anybox/graphics/container/Dxil.hpp>

AnyBox::Shader::DxilModule::DxilModule(std::span<const std::byte> code) {
    Graphics::Container::DxbcView container(code);
    auto part = container.FindPart(0x4C495844);
    if (!part) throw std::invalid_argument("Shader container has no DXIL program");
    Graphics::Container::DxilView program(part->data);
    auto bitcode = program.GetBitcode();

    module = LLVMBC::parseIR(context, bitcode.data(), bitcode.size());
    if (!module) throw std::runtime_error("DXIL bitcode parsing failed");
}

LLVMBC::Module& AnyBox::Shader::DxilModule::GetModule() const {
    return *module;
}
