#include "Fixture.hpp"
#include <Optimization/ConstantFolder.hpp>
#include <unordered_map>

std::uint64_t BufferTests::Evaluate(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& output) {
    using namespace ShaderRecompiler;
    ConstantFolder{}.Fold(program);
    ResolveControlFlowIdentities(program);
    ValidateProgram(program, true);
    std::unordered_map<const IrValue*, std::array<std::uint64_t, 4>> values;
    auto read = [&](const IrValue* value) {
        value = value->Resolve();
        if (value->HasImmediate()) {
            auto data = value->Type() == IrType::U64 ? value->ImmediateU64() : value->Type() == IrType::Bool ? static_cast<std::uint64_t>(value->ImmediateBool()) : value->ImmediateU32();
            return std::array<std::uint64_t, 4>{data};
        }
        return values.at(value);
    };
    for (auto* instruction : program.EntryBlock().Instructions()) {
        if (instruction->IsIdentity()) continue;
        auto access = AddressOpcodeInfoOf(instruction->Opcode());
        if (access.access == AddressAccess::Read) {
            std::array<std::uint64_t, 4> loaded{};
            if (read(instruction->Argument(3))[0]) {
                auto address = read(instruction->Argument(1))[0] | (read(instruction->Argument(2))[0] << 32);
                auto offset = address - 0x100000000;
                for (unsigned component = 0; component < access.components; ++component) {
                    for (unsigned byte = 0; byte < access.dataBits / 8; ++byte) {
                        auto location = offset + component * (access.dataBits / 8) + byte;
                        if (location >= 64) throw std::runtime_error("Lowered buffer load reads beyond the bound view");
                        loaded.at(component) |= ((location * 3 + 7) & 255) << (byte * 8);
                    }
                }
            }
            values[instruction] = loaded;
        } else {
            switch (instruction->Opcode()) {
                case IrOpcode::GetAddressResource:
                case IrOpcode::Return: break;
                case IrOpcode::CompositeConstructU64: values[instruction] = {read(instruction->Argument(0))[0] | (read(instruction->Argument(1))[0] << 32)}; break;
                case IrOpcode::CompositeExtractU32x2: values[instruction] = {read(instruction->Argument(0)).at(read(instruction->Argument(1))[0])}; break;
                case IrOpcode::ConvertU32U16: values[instruction] = {read(instruction->Argument(0))[0] & 65535}; break;
                default: throw std::runtime_error("Buffer test execution encountered an unexpected opcode");
            }
        }
    }
    return read(&output)[0];
}
