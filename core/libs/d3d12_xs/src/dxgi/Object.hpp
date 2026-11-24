#pragma once

#include "Device.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <limits>
#include <stdexcept>

namespace AnyBox::Graphics {
inline Guid ReadGuid(REFGUID value) noexcept {
    Guid result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

template<class TDerived, class TInterface, class TNative>
class DxgiObject : public TInterface {
public:
    HRESULT QueryInterface(REFIID interfaceId, void** object) override {
        if (!object) throw std::invalid_argument("DXGI interface output is null");
        *object = nullptr;
        auto id = ReadGuid(interfaceId);
        if (id != UnknownId && id != DxgiObjectId && !TDerived::Supports(id)) throw std::invalid_argument("Unsupported DXGI interface");
        AddRef();
        *object = static_cast<TDerived*>(this);
        return S_OK;
    }

    ULONG AddRef() override {
        auto previous = references.load(std::memory_order_relaxed);
        do {
            if (previous == 0 || previous == std::numeric_limits<ULONG>::max()) throw std::overflow_error("DXGI reference count is invalid");
        } while (!references.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
        return previous + 1;
    }

    ULONG Release() override {
        auto previous = references.fetch_sub(1, std::memory_order_acq_rel);
        if (previous == 0) throw std::logic_error("DXGI reference count underflows");
        if (previous == 1) delete static_cast<TDerived*>(this);
        return previous - 1;
    }

    HRESULT SetPrivateData(REFGUID key, UINT size, const void* data) override {
        if (size != 0 && !data) throw std::invalid_argument("DXGI private data is null");
        return check(host->SetPrivateData(key, size, data));
    }

    HRESULT SetPrivateDataInterface(REFGUID key, const IUnknown* object) override { return check(host->SetPrivateDataInterface(key, object)); }

    HRESULT GetPrivateData(REFGUID key, UINT* size, void* data) override {
        if (!size) throw std::invalid_argument("DXGI private data size output is null");
        return check(host->GetPrivateData(key, size, data));
    }

protected:
    explicit DxgiObject(ComOwner<TNative> native) : host(std::move(native)) {
        if (!host) throw std::invalid_argument("DXGI host object is null");
    }
    ~DxgiObject() = default;
    static HRESULT check(HRESULT result) {
        Windows::CheckHResult(result, "DXGI host operation");
        return result;
    }
    ComOwner<TNative> host;

private:
    std::atomic<ULONG> references{1};
};
}
