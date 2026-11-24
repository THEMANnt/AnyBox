#include "../file/Tests.hpp"

#include <limits>

void CheckStatusArray() {
    using namespace AnyBox::Tests;
    auto factory = GetFactory();
    void* output = nullptr;
    Require(factory->CreateStatusArray(37, "status test", AnyBox::Storage::StatusArrayId, &output) == 0, "Status array creation failed");
    Object<IStorageStatusArray> array(static_cast<IStorageStatusArray*>(output));
    for (std::uint32_t i = 0; i < 37; ++i) {
        Require(array->IsComplete(i) && array->GetHResult(i) == 0, "New status entry is not complete");
    }
    void* unknown = nullptr;
    Require(array->QueryInterface(AnyBox::Storage::UnknownId, &unknown) == 0 && unknown == array.get(), "Status array identity changed");
    Require(static_cast<IStorageUnknown*>(unknown)->Release() == 1, "Status array query did not retain ownership");
    ExpectException<std::out_of_range>([&] { array->IsComplete(37); });
    ExpectException<std::out_of_range>([&] { array->GetHResult(std::numeric_limits<std::uint32_t>::max()); });
    output = array.get();
    ExpectException<std::invalid_argument>([&] { factory->CreateStatusArray(0, nullptr, AnyBox::Storage::StatusArrayId, &output); });
    ExpectException<std::invalid_argument>([&] { factory->CreateStatusArray(1, nullptr, {}, &output); });
    Require(output == array.get(), "Rejected status array creation changed its output");
    ExpectException<std::invalid_argument>([&] { factory->CreateStatusArray(1, nullptr, AnyBox::Storage::StatusArrayId, nullptr); });
    output = nullptr;
    factory->CreateStatusArray(1, nullptr, AnyBox::Storage::StatusArrayId, &output);
    Object<IStorageStatusArray> unnamed(static_cast<IStorageStatusArray*>(output));
    Require(unnamed->IsComplete(0), "Unnamed status array is invalid");
}
