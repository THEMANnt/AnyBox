#ifndef ANYBOX_STORAGE_OPERATION_HPP
#define ANYBOX_STORAGE_OPERATION_HPP

#include "SignalOperation.hpp"
#include "../memory/View.hpp"
#include "../common/Handle.hpp"
#include "../file/ReadLease.hpp"
#include "../status/StatusArray.hpp"

#include <StorageTypes.hpp>

#include <variant>

namespace AnyBox::Storage {

struct ReadOperation {
    StorageRequest request;
    ReadLease file;
    MemoryView source;
    MemoryView destination;
    bool canceled = false;
    std::array<char, 64> name{};
};

class StatusOperation {
public:
    StatusOperation(StatusArray* array, std::uint32_t index);
    StatusOperation(StatusOperation&& other) noexcept;
    StatusOperation& operator=(StatusOperation&&) = delete;
    ~StatusOperation();
    void Complete(std::int32_t result);
    StorageStatusFailure GetFailure() const;

private:
    StatusArray* array;
    std::uint32_t index;
    StatusTicket ticket{};
    bool completed = false;
};

struct EventOperation {
    Handle event;
};

using Operation = std::variant<std::monostate, ReadOperation, StatusOperation, EventOperation, SignalOperation>;

}

#endif
