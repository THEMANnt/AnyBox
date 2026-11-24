#ifndef ANYBOX_STORAGE_SIGNAL_OPERATION_HPP
#define ANYBOX_STORAGE_SIGNAL_OPERATION_HPP

#include <StorageErrors.hpp>

namespace AnyBox::Storage {

class SignalOperation {
public:
    SignalOperation(ID3D12Fence* fence, std::uint64_t value);
    SignalOperation(SignalOperation&& other) noexcept;
    SignalOperation& operator=(SignalOperation&&) = delete;
    ~SignalOperation();
    std::int32_t Complete();
    StorageSignalFailure GetFailure() const;

private:
    ID3D12Fence* fence = nullptr;
    ID3D12Fence* original;
    std::uint64_t value;
};

}

#endif
