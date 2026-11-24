#include "Fixture.hpp"
#include "Fixtures.hpp"
#include "ComputeEntry.hpp"

namespace {
void SetEntry(ResourceTests::Fixture& fixture, unsigned threads, unsigned wave, bool ranged) {
    auto& context = fixture.Context();
    auto& module = fixture.Module();
    auto* name = context.construct<LLVMBC::MDString>(&module, "main");
    auto* model = fixture.Node({context.construct<LLVMBC::MDString>(&module, "cs"), fixture.Integer(6), fixture.Integer(5)});
    module.add_named_metadata("dx.shaderModel", context.construct<LLVMBC::NamedMDNode>(&module, "dx.shaderModel", LLVMBC::Vector<LLVMBC::MDNode*>{model}));
    auto* signature = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
    auto* function = context.construct<LLVMBC::Function>(signature, 100, module);
    auto* block = context.construct<LLVMBC::BasicBlock>(context);
    block->add_instruction(context.construct<LLVMBC::ReturnInst>(nullptr));
    function->set_basic_blocks({block});
    auto* pointer = context.construct<LLVMBC::ConstantAsMetadata>(&module, function);
    LLVMBC::Vector<LLVMBC::MDOperand*> properties{fixture.Integer(0), fixture.Integer(0x100000000ULL, 64), fixture.Integer(4), fixture.Node({fixture.Integer(threads), fixture.Integer(2), fixture.Integer(2)})};
    if (wave != 0) {
        properties.push_back(fixture.Integer(ranged ? 23 : 11));
        properties.push_back(ranged ? fixture.Node({fixture.Integer(wave), fixture.Integer(64), fixture.Integer(32)}) : fixture.Node({fixture.Integer(wave)}));
    }
    auto* tags = context.construct<LLVMBC::MDNode>(&module, std::move(properties));
    auto* entry = fixture.Node({pointer, name, fixture.Null(), fixture.Null(), tags});
    module.add_named_metadata("dx.entryPoints", context.construct<LLVMBC::NamedMDNode>(&module, "dx.entryPoints", LLVMBC::Vector<LLVMBC::MDNode*>{entry}));
}
}

void ResourceTests::CheckComputeEntries() {
    for (auto wave : {0u, 32u, 64u}) {
        Fixture fixture;
        SetEntry(fixture, 4, wave, false);
        auto entry = AnyBox::Shader::Dxil::ReadComputeEntry(fixture.Module());
        Fixtures::Require(entry.info.entryPoint == "main" && entry.info.threads == std::array<std::uint32_t, 3>{4, 2, 2} && entry.info.flags == 0x100000000ULL && entry.info.shaderModelMinor == 5);
        Fixtures::Require(entry.function != nullptr && entry.info.minimumWaveSize == wave && entry.info.maximumWaveSize == wave);
    }
    for (auto threads : {0u, 257u, 1025u}) {
        Fixture fixture;
        SetEntry(fixture, threads, 0, false);
        Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::ReadComputeEntry(fixture.Module()); });
    }
    {
        Fixture fixture;
        SetEntry(fixture, 4, 7, false);
        Fixtures::Reject<std::runtime_error>([&] { (void)AnyBox::Shader::Dxil::ReadComputeEntry(fixture.Module()); });
    }
    {
        Fixture fixture;
        SetEntry(fixture, 4, 16, true);
        auto entry = AnyBox::Shader::Dxil::ReadComputeEntry(fixture.Module());
        Fixtures::Require(entry.info.minimumWaveSize == 16 && entry.info.maximumWaveSize == 64 && entry.info.preferredWaveSize == 32);
    }
}
