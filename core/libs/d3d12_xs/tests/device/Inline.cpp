#include "Tests.hpp"
#include "../../src/list/List.hpp"

namespace {
template<class TValue>
TValue Read(void* object, std::size_t offset) {
    TValue result;
    std::memcpy(&result, static_cast<std::byte*>(object) + offset, sizeof(result));
    return result;
}

void SetWritePointer(void* object, std::uint32_t* pointer) { std::memcpy(static_cast<std::byte*>(object) + 24, &pointer, sizeof(pointer)); }
}

void AnyBox::Graphics::Tests::CheckInlineCommands(Device* device) {
    for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE}) {
        void* output = nullptr;
        Require(Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, type, AllocatorId, &output) == 0 && output, "Inline ABI allocator creation failed");
        auto* allocator = static_cast<Allocator*>(output);
        XboxListDescription description{type, 2, 1, 0};
        Require(Method<decltype(&Device::CreateCommandListX)>(device, 73)(device, &description, allocator, nullptr, XboxGraphicsListId, &output) == 0 && output, "Inline ABI list creation failed");
        auto* list = static_cast<CommandList*>(output);
        auto close = Method<decltype(&CommandList::Close)>(list, 9);
        auto addRef = Method<decltype(&CommandList::AddRef)>(list, 1);
        auto release = Method<decltype(&CommandList::Release)>(list, 2);
        Require(Read<std::uint32_t>(list, 16) == static_cast<std::uint32_t>(type), "Xbox command list type header changed");
        Require(Read<std::uint32_t>(list, 12) == 1 && addRef(list) == 2 && Read<std::uint32_t>(list, 12) == 2, "Xbox command list reference header changed");
        Require(release(list) == 1, "Inline ABI interface reference was not released");
        auto* begin = Read<std::uint32_t*>(list, 24);
        auto* limit = Read<std::uint32_t*>(list, 40);
        Require(begin && limit > begin && limit == Read<std::uint32_t*>(list, 48), "Xbox command stream bounds are invalid");

        begin[0] = 0xDEAD0000;
        SetWritePointer(list, begin + 1);
        Reject<std::runtime_error>([&] { close(list); });
        Require(!list->IsClosed(), "Unknown inline command stream was accepted");
        auto top = Method<decltype(&CommandList::SyncTopOfPipeX)>(list, 131);
        Reject<std::runtime_error>([&] { top(list, 0xC380, 0x20000, 0, (std::uint64_t{1} << 48) - 256); });
        SetWritePointer(list, reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uintptr_t>(limit) + 65 * 4));
        Reject<std::logic_error>([&] { close(list); });
        SetWritePointer(list, begin);
        Require(close(list) == 0, "Empty inline command stream close failed");
        Require(release(list) == 0, "Inline ABI command list was not released");
        Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Inline ABI allocator was not released");
    }
}
