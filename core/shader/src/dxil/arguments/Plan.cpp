#include <anybox/shader/BufferArguments.hpp>
#include <algorithm>
#include <stdexcept>

using namespace AnyBox::Shader;

BufferArgumentPlan::BufferArgumentPlan(std::span<const BufferBinding> bindings, std::uint32_t firstUserRegister) : userRegisterCount(firstUserRegister) {
    if (firstUserRegister > UserRegisterCapacity) throw std::invalid_argument("Buffer argument register base exceeds the ABI capacity");
    for (const auto& binding : bindings) {
        bool constant = binding.resourceClass == ResourceClass::ConstantBuffer;
        if (binding.resourceClass != ResourceClass::ShaderResource && binding.resourceClass != ResourceClass::UnorderedAccess && !constant) throw std::invalid_argument("Buffer argument resource class is invalid");
        if (constant ? binding.kind != BufferKind::Constant : binding.kind != BufferKind::Raw && binding.kind != BufferKind::Structured) throw std::invalid_argument("Buffer argument kind disagrees with its resource class");
        bool structured = binding.kind == BufferKind::Structured;
        if (structured ? binding.stride == 0 || binding.stride > 2048 || binding.stride % 4 != 0 : binding.stride != 0) throw std::invalid_argument("Buffer argument structure stride is invalid");
        if (constant && (binding.byteSize == 0 || binding.byteSize > 65536)) throw std::invalid_argument("Buffer argument constant size is invalid");
        if (binding.hasCounter || binding.rasterizerOrdered) throw std::runtime_error("Buffer argument counters and ordered access are not implemented");
        if (binding.count == 0 || binding.count > (UserRegisterCapacity - userRegisterCount) / WordsPerBuffer) throw std::length_error("Buffer argument array exceeds the ABI capacity");
        auto end = static_cast<std::uint64_t>(binding.firstRegister) + binding.count;
        if (end > static_cast<std::uint64_t>(UINT32_MAX) + 1) throw std::invalid_argument("Buffer argument register range overflows");
        for (const auto& previous : ranges) {
            const auto& other = previous.binding;
            if (other.resourceClass != binding.resourceClass) continue;
            if (other.rangeId == binding.rangeId) throw std::invalid_argument("Buffer argument range identifier is duplicated");
            auto otherEnd = static_cast<std::uint64_t>(other.firstRegister) + other.count;
            if (other.space == binding.space && binding.firstRegister < otherEnd && other.firstRegister < end) throw std::invalid_argument("Buffer argument register ranges overlap");
        }
        ranges.push_back({binding, userRegisterCount});
        userRegisterCount += binding.count * WordsPerBuffer;
    }
}

std::span<const BufferArgumentRange> BufferArgumentPlan::GetRanges() const { return ranges; }

std::uint32_t BufferArgumentPlan::GetUserRegisterCount() const { return userRegisterCount; }

const BufferArgumentRange& BufferArgumentPlan::Find(const BufferBinding& binding) const {
    auto found = std::ranges::find_if(ranges, [&](const auto& range) { return range.binding == binding; });
    if (found == ranges.end()) throw std::invalid_argument("Buffer binding is absent from the argument plan");
    return *found;
}
