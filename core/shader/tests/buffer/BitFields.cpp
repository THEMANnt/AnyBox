#include "FloatFixture.hpp"
#include <algorithm>

namespace {
std::uint32_t Reference(unsigned opcode, std::uint32_t source, std::uint32_t destination, unsigned width, unsigned offset) {
    offset &= 31;
    width = std::min(width & 31, 32 - offset);
    if (width == 0) return opcode == 53 ? destination : 0;
    auto mask = (std::uint32_t{1} << width) - 1;
    if (opcode == 53) return (destination & ~(mask << offset)) | ((source & mask) << offset);
    auto result = (source >> offset) & mask;
    if (opcode == 51 && (result & (std::uint32_t{1} << (width - 1)))) result |= ~mask;
    return result;
}
}

void BufferTests::CheckBitFields(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0, 1, 7, 16, 31, 32, 33, 63, 64, UINT32_MAX, 0x80000000, 0xF1234567, 0x12345678, 0x7FFFFFFF, 0xAAAAAAAA, 0x55555555};
    for (unsigned width = 0; width < 10; ++width) {
        FloatFixture fixture;
        std::array<LLVMBC::Value*, 16> dynamic{};
        auto* type = fixture.IntegerType(32);
        for (unsigned index = 0; index < dynamic.size(); ++index) dynamic[index] = fixture.Make<LLVMBC::CastInst>(type, fixture.Argument(index), LLVMBC::Instruction::BitCast);
        for (unsigned offset = 0; offset < 10; ++offset) {
            for (unsigned opcode : {51u, 52u, 53u}) {
                unsigned value = 10 + (width + offset) % 6;
                unsigned destination = 10 + (width * 3 + offset) % 6;
                LLVMBC::Vector<LLVMBC::Value*> arguments{fixture.Integer(32, opcode), dynamic[width], dynamic[offset], dynamic[value]};
                if (opcode == 53) arguments.push_back(dynamic[destination]);
                auto* call = fixture.Call(opcode == 53 ? "dx.op.quaternary.i32" : "dx.op.tertiary.i32", type, std::move(arguments));
                fixture.Expect(*call, Reference(opcode, input[value], input[destination], input[width], input[offset]));
            }
        }
        auto* invalid = fixture.Call("dx.op.tertiary.i32", type, {fixture.Integer(32, 50), dynamic[0], dynamic[1], dynamic[2]});
        fixture.Reject(*invalid);
        fixture.Run(context, input);
    }
}
