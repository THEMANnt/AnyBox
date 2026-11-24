#pragma once

#include <IntermediateRepresentation/IrBuilder.hpp>

namespace NativeMaskTests {
enum class Mode { Narrow, Widen, UnguardedWords, UnguardedBoolean, SwappedHalves, WrongPartition, Changing };

struct Fixture {
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrValue* mask = nullptr;
    ShaderRecompiler::IrValue* read = nullptr;
    ShaderRecompiler::IrValue* payload = nullptr;
    ShaderRecompiler::IrValue* value = nullptr;
    ShaderRecompiler::IrValue* chosen = nullptr;

    Fixture(std::uint32_t waveSize, Mode mode, bool bitWords = false) {
        using namespace ShaderRecompiler;
        program.SetWaveSize(waveSize);
        auto& entry = program.CreateBlock();
        auto& header = program.CreateBlock();
        auto& body = program.CreateBlock();
        auto& exit = program.CreateBlock();
        program.SetEntryBlock(entry);
        program.BlockOrder() = {&entry, &header, &body, &exit};
        entry.AddBranch(&header);
        header.AddBranch(&body);
        header.AddBranch(&exit);
        body.AddBranch(&header);
        auto& metadata = program.Metadata().blockInfo;
        metadata.resize(4);
        for (unsigned index = 0; index < 4; ++index) metadata[index].id = index;
        metadata[0].terminator.kind = metadata[2].terminator.kind = TerminatorKind::Branch;
        metadata[0].terminator.trueBlock = metadata[2].terminator.trueBlock = 1;
        metadata[1].terminator.kind = TerminatorKind::ConditionalBranch;
        metadata[1].terminator.loopHeader = true;
        metadata[1].terminator.trueBlock = 2;
        metadata[1].terminator.falseBlock = metadata[1].terminator.mergeBlock = 3;
        metadata[1].terminator.continueBlock = 2;
        IrBuilder builder(program);
        builder.SetInsertionPoint(entry);
        auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
        auto& lowBits = builder.Constant(0xFFFFFE00U);
        auto& highBits = builder.Constant(0xFF00FFFFU);
        auto& initialHalf = builder.ULessThan(lane, builder.Constant(32));
        auto& initialWord = waveSize == 64 ? builder.Select(initialHalf, lowBits, highBits) : lowBits;
        auto& initialSelector = builder.BitwiseAnd(lane, builder.Constant(31));
        auto& bitCondition = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(initialWord, initialSelector), builder.Constant(1)), builder.Constant(0));
        auto& condition = bitWords ? bitCondition : builder.UGreaterThan(lane, builder.Constant(8));
        auto& initial = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {mode == Mode::UnguardedWords ? &builder.ConstantBool(true) : &condition});
        auto* lowInitial = &builder.CompositeExtract(initial, 0);
        auto* highInitial = &builder.CompositeExtract(initial, 1);
        if (bitWords) {
            lowInitial = mode == Mode::UnguardedWords ? &builder.Constant(UINT32_MAX) : &lowBits;
            highInitial = mode == Mode::UnguardedWords ? &builder.Constant(UINT32_MAX) : &highBits;
        }
        builder.SetInsertionPoint(header);
        auto& currentMask = builder.Emit(IrOpcode::Phi, IrType::Bool, {});
        auto& low = builder.Emit(IrOpcode::Phi, IrType::U32, {});
        auto& high = builder.Emit(IrOpcode::Phi, IrType::U32, {});
        metadata[1].condition = &currentMask;
        builder.SetInsertionPoint(body);
        auto& guard = mode == Mode::Changing ? builder.LogicalNot(currentMask) : condition;
        auto& currentPayload = builder.ShiftLeftLogical(builder.IAdd(lane, builder.Constant(1)), builder.Constant(5));
        auto& currentValue = builder.Select(guard, currentPayload, builder.Constant(3));
        auto& currentRead = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&currentValue, &currentMask});
        auto& currentChosen = builder.LogicalAnd(currentMask, builder.IEqual(currentRead, currentPayload));
        auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&currentChosen});
        auto& lowRemoved = builder.BitwiseNot(builder.CompositeExtract(ballot, 0));
        auto& highRemoved = builder.BitwiseNot(builder.CompositeExtract(ballot, 1));
        auto& lowNext = mode == Mode::Widen ? builder.BitwiseOr(low, lowRemoved) : builder.BitwiseAnd(low, lowRemoved);
        auto& highNext = mode == Mode::Widen ? builder.BitwiseOr(high, highRemoved) : builder.BitwiseAnd(high, highRemoved);
        auto& lowHalf = builder.ULessThan(lane, builder.Constant(mode == Mode::WrongPartition ? 16 : 32));
        auto& selected = mode == Mode::SwappedHalves ? builder.Select(lowHalf, highNext, lowNext) : builder.Select(lowHalf, lowNext, highNext);
        auto& word = waveSize == 64 ? selected : lowNext;
        auto& selector = builder.BitwiseAnd(lane, builder.Constant(31));
        auto& next = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(word, selector), builder.Constant(1)), builder.Constant(0));
        currentMask.AddPhiOperand(&entry, mode == Mode::UnguardedBoolean ? &builder.ConstantBool(true) : &condition);
        currentMask.AddPhiOperand(&body, &next);
        low.AddPhiOperand(&entry, lowInitial);
        low.AddPhiOperand(&body, &lowNext);
        high.AddPhiOperand(&entry, highInitial);
        high.AddPhiOperand(&body, &highNext);
        this->mask = &currentMask;
        this->read = &currentRead;
        this->payload = &currentPayload;
        this->value = &currentValue;
        this->chosen = &currentChosen;
        ValidateProgram(program, true);
    }
};
}
