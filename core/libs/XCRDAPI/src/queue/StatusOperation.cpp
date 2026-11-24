#include "Operation.hpp"

#include <utility>

AnyBox::Storage::StatusOperation::StatusOperation(StatusArray* array, std::uint32_t index) : array(array), index(index) {
    array->AddRef();
    try {
        ticket = array->GetEntry(index).Begin();
    } catch (...) {
        array->Release();
        throw;
    }
}

AnyBox::Storage::StatusOperation::StatusOperation(StatusOperation&& other) noexcept : array(std::exchange(other.array, nullptr)), index(other.index), ticket(other.ticket), completed(other.completed) {}

AnyBox::Storage::StatusOperation::~StatusOperation() {
    if (!array) return;
    if (!completed) array->GetEntry(index).Cancel(ticket);
    array->Release();
}

void AnyBox::Storage::StatusOperation::Complete(std::int32_t result) {
    array->GetEntry(index).Complete(ticket, result);
    completed = true;
}

StorageStatusFailure AnyBox::Storage::StatusOperation::GetFailure() const { return {array, index}; }
