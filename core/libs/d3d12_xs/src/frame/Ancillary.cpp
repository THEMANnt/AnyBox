#include "Ancillary.hpp"
#include <cstring>
#include <string>
#include <string_view>

namespace {
bool IsSemaphore(HANDLE handle) {
    using Query = LONG (WINAPI *)(HANDLE, ULONG, void*, ULONG, ULONG*);
    static auto query = [] {
        auto module = GetModuleHandleW(L"ntdll.dll");
        if (!module) AnyBox::Windows::ThrowLastError("GetModuleHandleW frame ancillary metadata");
        auto address = GetProcAddress(module, "NtQueryObject");
        if (!address) AnyBox::Windows::ThrowLastError("GetProcAddress frame ancillary metadata");
        Query result;
        std::memcpy(&result, &address, sizeof(result));
        return result;
    }();
    struct BasicInformation {
        ULONG attributes;
        ULONG access;
        ULONG handles;
        ULONG pointers;
        ULONG reserved[10];
    };
    BasicInformation basic{};
    auto status = query(handle, 0, &basic, sizeof(basic), nullptr);
    if (status < 0) throw std::runtime_error("NtQueryObject frame ancillary access failed: " + std::to_string(status));
    if ((basic.access & 2) == 0) throw std::invalid_argument("Frame ancillary handle does not permit signaling");
    alignas(void*) std::array<std::byte, 1024> buffer{};
    status = query(handle, 2, buffer.data(), static_cast<ULONG>(buffer.size()), nullptr);
    if (status < 0) throw std::runtime_error("NtQueryObject frame ancillary metadata failed: " + std::to_string(status));
    struct TypeName {
        USHORT length;
        USHORT capacity;
        const wchar_t* data;
    };
    TypeName name;
    std::memcpy(&name, buffer.data(), sizeof(name));
    auto address = reinterpret_cast<std::uintptr_t>(name.data);
    auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    if (name.length > name.capacity || name.length % sizeof(wchar_t) != 0 || address % alignof(wchar_t) != 0 || address < begin || address > begin + buffer.size() || name.length > begin + buffer.size() - address) throw std::runtime_error("Frame ancillary object type is malformed");
    std::wstring_view type(name.data, name.length / sizeof(wchar_t));
    if (type == L"Event") return false;
    if (type == L"Semaphore") return true;
    throw std::invalid_argument("Frame ancillary signal requires an event or semaphore");
}
}

AnyBox::Graphics::FrameSignals::FrameSignals(const FrameAncillary* ancillary) {
    if (!ancillary) return;
    if (ancillary->count > signals.size() || (ancillary->count != 0 && !ancillary->handles)) throw std::invalid_argument("Frame ancillary signal array is invalid");
    for (std::uint32_t index = 0; index < ancillary->count; ++index) {
        auto source = ancillary->handles[index];
        if (!source || source == INVALID_HANDLE_VALUE) throw std::invalid_argument("Frame ancillary signal handle is invalid");
        HANDLE duplicate = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) Windows::ThrowLastError("DuplicateHandle Xbox frame signal");
        signals[index].handle = OwnFrameHandle(duplicate, "Own Xbox frame ancillary signal");
        signals[index].semaphore = IsSemaphore(duplicate);
        ++count;
    }
}

void AnyBox::Graphics::FrameSignals::Signal() const {
    for (std::uint32_t index = 0; index < count; ++index) signals[index].Signal();
}

void AnyBox::Graphics::FrameSignal::Signal() const {
    if (semaphore) SignalFrameSemaphore(handle.get());
    else SignalFrameEvent(handle.get());
}
