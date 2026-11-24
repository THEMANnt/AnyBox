#include "Tests.hpp"

#include <cstdint>

void AnyBox::Tests::CheckFactory() {
    auto first = GetFactory();
    auto second = GetFactory();
    Require(first.get() == second.get(), "DirectStorage factory is not a singleton");
    Require(first->AddRef() == 3 && first->Release() == 2, "DirectStorage reference counts are invalid");
    void* unknown = nullptr;
    Require(first->QueryInterface(Storage::UnknownId, &unknown) == 0 && unknown == first.get(), "Factory IUnknown identity changed");
    static_cast<IStorageUnknown*>(unknown)->Release();
    void* output = first.get();
    ExpectException<std::invalid_argument>([&] { DStorageGetFactory({}, &output); });
    Require(output == first.get(), "Failed factory query changed its output");
    ExpectException<std::invalid_argument>([] { DStorageGetFactory(Storage::FactoryId, nullptr); });
    ExpectException<std::invalid_argument>([&] { first->SetCpuAffinity(0); });
    DWORD_PTR processMask;
    DWORD_PTR systemMask;
    Require(GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) != 0, "Factory affinity test failed");
    Require(first->SetCpuAffinity(processMask) == 0, "Factory rejected the process affinity mask");
    ExpectException<std::invalid_argument>([&] { first->SetDebugFlags(0x80000000); });
    first->SetDebugFlags(0);
    Require(first->SetStagingBufferSize(0x2000000) == 0, "Factory staging allocation failed");
    Require(first->SetStagingBufferSize(0) == 0, "Factory staging release failed");
}
