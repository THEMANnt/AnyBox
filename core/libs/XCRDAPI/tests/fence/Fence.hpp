#ifndef ANYBOX_STORAGE_TEST_FENCE_HPP
#define ANYBOX_STORAGE_TEST_FENCE_HPP

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace AnyBox::Tests {

class TestFence final : public ID3D12Fence {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interfaceId, void** output) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override;
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT, const void*) override;
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID, const IUnknown*) override;
    HRESULT STDMETHODCALLTYPE SetName(LPCWSTR) override;
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID, void**) override;
    UINT64 STDMETHODCALLTYPE GetCompletedValue() override;
    HRESULT STDMETHODCALLTYPE SetEventOnCompletion(UINT64, HANDLE) override;
    HRESULT STDMETHODCALLTYPE Signal(UINT64 value) override;
    void SetResult(HRESULT result);
    void SetInterfaceSupported(bool supported);
    void ExpectAffinity(std::uint64_t affinity);
    void ExpectRead(const std::byte* address, std::byte value);
    ULONG GetReferenceCount() const;
    std::uint32_t GetSignalCount() const;

private:
    std::atomic<ULONG> references{1};
    std::atomic<UINT64> completed{0};
    std::atomic<std::uint32_t> signalCount{0};
    HRESULT result = S_OK;
    bool supported = true;
    std::uint64_t expectedAffinity = 0;
    const std::byte* address = nullptr;
    std::byte expected{};
};

}

#endif
