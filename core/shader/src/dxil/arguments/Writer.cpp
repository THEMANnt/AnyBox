#include <anybox/shader/RootArgumentWriter.hpp>
#include <algorithm>
#include <array>
#include <stdexcept>

using namespace AnyBox::Shader;
using namespace AnyBox::Graphics::Container;

RootArgumentWriter::RootArgumentWriter(const RootArgumentPlan& plan, std::span<const std::uint32_t> registers) {
    if (registers.size() > BufferArgumentPlan::UserRegisterCapacity) throw std::length_error("Root writer register count exceeds the ABI capacity");
    wordCount = static_cast<std::uint32_t>(registers.size());
    std::array<bool, BufferArgumentPlan::UserRegisterCapacity> used{};
    for (std::uint32_t destination = 0; destination < wordCount; ++destination) {
        auto source = registers[destination];
        if (source >= used.size() || used[source]) throw std::invalid_argument("Root writer register selection is invalid or duplicated");
        used[source] = true;
        auto found = std::ranges::find_if(plan.GetRanges(), [&](const auto& range) { return source >= range.firstUserRegister && source - range.firstUserRegister < range.wordCount; });
        if (found == plan.GetRanges().end()) throw std::invalid_argument("Root writer register has no argument mapping");
        auto relative = source - found->firstUserRegister;
        auto element = found->kind == RootArgumentKind::Constants ? 0 : relative / BufferArgumentPlan::WordsPerBuffer;
        auto step = std::ranges::find_if(steps, [&](const auto& value) { return value.range.firstUserRegister == found->firstUserRegister && value.element == element; });
        if (step == steps.end()) {
            steps.push_back({*found, element, {}});
            step = steps.end() - 1;
        }
        step->copies.push_back({found->kind == RootArgumentKind::Constants ? relative : relative % BufferArgumentPlan::WordsPerBuffer, destination});
    }
}

void RootArgumentWriter::Write(const Graphics::Commands::RootState& state, IRootResources& resources, std::span<std::uint32_t> destination) const {
    if (destination.size() != wordCount) throw std::invalid_argument("Root writer destination size is invalid");
    for (const auto& step : steps) {
        const auto& range = step.range;
        auto kind = state.GetKind(range.parameter);
        if (range.kind == RootArgumentKind::Constants) {
            if (kind != RootParameterKind::Constants || state.GetWordCount(range.parameter) != range.wordCount) throw std::invalid_argument("Root writer constant layout differs from its plan");
            for (const auto& copy : step.copies) destination[copy.destination] = state.Read(range.parameter, copy.source, 1).front();
            continue;
        }
        RootBufferValue buffer;
        if (range.kind == RootArgumentKind::Table) {
            if (kind != RootParameterKind::Table) throw std::invalid_argument("Root writer table layout differs from its plan");
            buffer = resources.ResolveTable(range.binding, state.Read(range.parameter, 0, 1).front(), range.tableOffset + step.element);
        } else {
            auto expected = range.binding.resourceClass == ResourceClass::ShaderResource ? RootParameterKind::ShaderResource : range.binding.resourceClass == ResourceClass::UnorderedAccess ? RootParameterKind::UnorderedAccess : RootParameterKind::ConstantBuffer;
            if (kind != expected) throw std::invalid_argument("Root writer descriptor layout differs from its plan");
            auto words = state.Read(range.parameter, 0, 2);
            auto address = words[0] | (static_cast<std::uint64_t>(words[1]) << 32);
            buffer = resources.ResolveDescriptor(range.binding, address);
        }
        std::array words{static_cast<std::uint32_t>(buffer.address), static_cast<std::uint32_t>(buffer.address >> 32), static_cast<std::uint32_t>(buffer.byteLength), static_cast<std::uint32_t>(buffer.byteLength >> 32)};
        for (const auto& copy : step.copies) destination[copy.destination] = words[copy.source];
    }
}
