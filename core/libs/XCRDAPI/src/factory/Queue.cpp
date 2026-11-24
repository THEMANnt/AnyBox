#include "Factory.hpp"
#include "../queue/Queue.hpp"

#include <stdexcept>

std::int32_t AnyBox::Storage::Factory::CreateQueue(const StorageQueueDescription* description, const Guid& interfaceId, void** queue) {
    if (!description) throw std::invalid_argument("DirectStorage queue description is null");
    StorageQueueDescription1 extended{*description, 0};
    return CreateQueue1(&extended, interfaceId, queue);
}

std::int32_t AnyBox::Storage::Factory::CreateQueue1(const StorageQueueDescription1* description, const Guid& interfaceId, void** queue) {
    if (!description || !queue) throw std::invalid_argument("DirectStorage queue description or output is null");
    if (interfaceId != QueueId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage queue interface");
    auto* created = new Queue(*description);
    *queue = static_cast<IStorageQueue*>(created);
    return 0;
}
