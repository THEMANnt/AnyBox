#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::waveReduction(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 4 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL floating wave reduction has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || GetType(*instruction.getArgOperand(1)) != IrType::F32 || width(*instruction.getArgOperand(2)) != 8 || width(*instruction.getArgOperand(3)) != 8) throw std::runtime_error("DXIL floating wave reduction has invalid operand types");
    auto constant = [&](unsigned index) {
        auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
        if (!value) throw std::runtime_error("DXIL wave reduction control operands must be constant");
        return value->getUniqueInteger().getZExtValue();
    };
    if (constant(0) != 119 || constant(3) != 0) throw std::runtime_error("DXIL floating wave reduction opcode or signedness is not implemented");
    auto operation = constant(2);
    if (operation != 2 && operation != 3) throw std::runtime_error("DXIL floating wave reduction operation is not implemented");
    if (program.WaveSize() != 32) throw std::runtime_error("DXIL floating wave reduction requires a 32-lane wave; other wave sizes are not implemented");

    auto& bits = builder.BitCastU32(Get(*instruction.getArgOperand(1)));
    auto& magnitude = builder.BitwiseAnd(bits, builder.Constant(0x7FFFFFFF));
    auto& sign = builder.BitwiseAnd(bits, builder.Constant(0x80000000U));
    auto& subnormal = builder.ULessThan(magnitude, builder.Constant(0x00800000));
    auto& flushed = builder.Select(subnormal, sign, bits);
    auto& negative = builder.INotEqual(sign, builder.Constant(0));
    auto& ordered = builder.Select(negative, builder.BitwiseNot(flushed), builder.BitwiseXor(flushed, builder.Constant(0x80000000U)));
    auto& nan = builder.UGreaterThan(magnitude, builder.Constant(0x7F800000));
    auto& sentinel = builder.Constant(operation == 2 ? UINT32_MAX : 0);
    auto& key = builder.Select(nan, sentinel, ordered);
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&builder.ConstantBool(true)});
    auto& active = builder.CompositeExtract(ballot, 0);
    auto& first = builder.Emit(IrOpcode::FindILsb32, IrType::U32, {&active});
    IrValue* reduced = &key;
    for (unsigned lane = 0; lane < 32; ++lane) {
        auto& bit = builder.BitwiseAnd(active, builder.Constant(std::uint32_t{1} << lane));
        auto& present = builder.INotEqual(bit, builder.Constant(0));
        auto& selected = builder.Select(present, builder.Constant(lane), first);
        auto& value = builder.Emit(IrOpcode::ReadLane, IrType::U32, {&key, &selected});
        reduced = &builder.Emit(operation == 2 ? IrOpcode::UMin32 : IrOpcode::UMax32, IrType::U32, {reduced, &value});
    }
    auto& positive = builder.INotEqual(builder.BitwiseAnd(*reduced, builder.Constant(0x80000000U)), builder.Constant(0));
    auto& decoded = builder.Select(positive, builder.BitwiseXor(*reduced, builder.Constant(0x80000000U)), builder.BitwiseNot(*reduced));
    auto& result = builder.Select(builder.IEqual(*reduced, sentinel), builder.Constant(0x7FC00000), decoded);
    return builder.BitCastF32(result);
}
