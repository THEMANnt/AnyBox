#pragma once

#include "../../src/host/Com.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <d3d12sdklayers.h>

namespace AnyBox::Graphics::Tests {
class PrivateDataDiagnostics {
public:
    explicit PrivateDataDiagnostics(ID3D12Device* device) {
        constexpr GUID interfaceId{0x0742A90B, 0xC387, 0x483F, {0xB9, 0x46, 0x30, 0xA7, 0xE4, 0xE6, 0x14, 0x58}};
        void* output = nullptr;
        auto result = device->QueryInterface(interfaceId, &output);
        if (result == E_NOINTERFACE) return;
        Windows::CheckHResult(result, "Query private data reference diagnostics");
        queue.reset(static_cast<ID3D12InfoQueue*>(output));
        if (!queue) throw std::runtime_error("Private data reference diagnostics are null");
        D3D12_MESSAGE_ID expected[]{D3D12_MESSAGE_ID_GETPRIVATEDATA_MOREDATA, D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS};
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = 2;
        filter.DenyList.pIDList = expected;
        Windows::CheckHResult(queue->PushStorageFilter(&filter), "Filter expected private data reference diagnostics");
    }
    ~PrivateDataDiagnostics() { if (queue) queue->PopStorageFilter(); }
    PrivateDataDiagnostics(const PrivateDataDiagnostics&) = delete;
    PrivateDataDiagnostics& operator=(const PrivateDataDiagnostics&) = delete;

private:
    ComOwner<ID3D12InfoQueue> queue;
};
}
