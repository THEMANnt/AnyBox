#include "Fence.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <cstring>
#include <stdexcept>

HRESULT AnyBox::Tests::TestFence::QueryInterface(REFIID interfaceId, void** output) {
    if (!output) return E_POINTER;
    constexpr GUID fenceId{0x0A753DCF, 0xC4D8, 0x4B91, {0xAD, 0xF6, 0xBE, 0x5A, 0x60, 0xD9, 0x5A, 0x76}};
    constexpr GUID unknownId{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    *output = nullptr;
    if (!supported || (std::memcmp(&interfaceId, &fenceId, sizeof(GUID)) != 0 && std::memcmp(&interfaceId, &unknownId, sizeof(GUID)) != 0)) return E_NOINTERFACE;
    *output = static_cast<ID3D12Fence*>(this);
    AddRef();
    return S_OK;
}

ULONG AnyBox::Tests::TestFence::AddRef() { return ++references; }
ULONG AnyBox::Tests::TestFence::Release() {
    auto count = --references;
    if (count == 0) delete this;
    return count;
}

HRESULT AnyBox::Tests::TestFence::GetPrivateData(REFGUID, UINT*, void*) { throw std::logic_error("Unexpected fence GetPrivateData"); }
HRESULT AnyBox::Tests::TestFence::SetPrivateData(REFGUID, UINT, const void*) { throw std::logic_error("Unexpected fence SetPrivateData"); }
HRESULT AnyBox::Tests::TestFence::SetPrivateDataInterface(REFGUID, const IUnknown*) { throw std::logic_error("Unexpected fence SetPrivateDataInterface"); }
HRESULT AnyBox::Tests::TestFence::SetName(LPCWSTR) { throw std::logic_error("Unexpected fence SetName"); }
HRESULT AnyBox::Tests::TestFence::GetDevice(REFIID, void**) { throw std::logic_error("Unexpected fence GetDevice"); }
UINT64 AnyBox::Tests::TestFence::GetCompletedValue() { return completed.load(); }
HRESULT AnyBox::Tests::TestFence::SetEventOnCompletion(UINT64, HANDLE) { throw std::logic_error("Unexpected fence SetEventOnCompletion"); }

HRESULT AnyBox::Tests::TestFence::Signal(UINT64 value) {
    ++signalCount;
    if (address && *address != expected) return E_FAIL;
    if (expectedAffinity != 0) {
        using QueryAffinity = BOOL (WINAPI*)(HANDLE, PGROUP_AFFINITY);
        static const auto query = [] {
            auto module = GetModuleHandleW(L"kernel32.dll");
            if (!module) AnyBox::Windows::ThrowLastError("GetModuleHandleW fence affinity test");
            auto address = GetProcAddress(module, "GetThreadGroupAffinity");
            if (!address) AnyBox::Windows::ThrowLastError("GetThreadGroupAffinity fence test");
            QueryAffinity function;
            static_assert(sizeof(function) == sizeof(address));
            std::memcpy(&function, &address, sizeof(function));
            return function;
        }();
        GROUP_AFFINITY affinity{};
        if (!query(GetCurrentThread(), &affinity)) AnyBox::Windows::ThrowLastError("GetThreadGroupAffinity fence test");
        if (affinity.Mask != expectedAffinity) return E_FAIL;
        if ((expectedAffinity & (1ULL << GetCurrentProcessorNumber())) == 0) return E_FAIL;
    }
    if (FAILED(result)) return result;
    completed.store(value);
    return result;
}

void AnyBox::Tests::TestFence::SetInterfaceSupported(bool value) { supported = value; }
void AnyBox::Tests::TestFence::SetResult(HRESULT value) { result = value; }
void AnyBox::Tests::TestFence::ExpectAffinity(std::uint64_t value) { expectedAffinity = value; }
void AnyBox::Tests::TestFence::ExpectRead(const std::byte* input, std::byte value) { address = input; expected = value; }
ULONG AnyBox::Tests::TestFence::GetReferenceCount() const { return references.load(); }
std::uint32_t AnyBox::Tests::TestFence::GetSignalCount() const { return signalCount.load(); }
