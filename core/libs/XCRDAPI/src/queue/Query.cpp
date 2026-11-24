#include "Queue.hpp"

#include <stdexcept>

void AnyBox::Storage::Queue::Query(StorageQueueInformation* information) {
    if (!information) throw std::invalid_argument("DirectStorage queue information output is null");
    std::lock_guard lock(mutex);
    if (closing) throw std::logic_error("DirectStorage queue is closing");

    information->description = description.description;
    information->emptySlotCount = static_cast<std::uint16_t>(operations.size() - count - 1);
    information->requestCountUntilAutoSubmit = static_cast<std::uint16_t>(operations.size() / 2 - (count - submitted));
}

void AnyBox::Storage::Queue::Query1(StorageQueueInformation1* information) {
    if (!information) throw std::invalid_argument("DirectStorage queue information output is null");
    std::lock_guard lock(mutex);
    if (closing) throw std::logic_error("DirectStorage queue is closing");

    information->description = description;
    information->emptySlotCount = static_cast<std::uint16_t>(operations.size() - count - 1);
    information->requestCountUntilAutoSubmit = static_cast<std::uint16_t>(operations.size() / 2 - (count - submitted));
}
