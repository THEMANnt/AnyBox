#include "Pool.hpp"

#include <exception>
#include <set>
#include <stdexcept>

void AnyBox::XMem::Pool::Map(std::span<void* const> addresses, const std::uint64_t* frames) {
    std::lock_guard lock(mutex);
    std::vector<MappingChange> changes;
    changes.reserve(addresses.size());
    std::set<std::uintptr_t> requested;
    std::map<std::uintptr_t, Page> added;
    for (std::size_t i = 0; i < addresses.size(); ++i) {
        auto address = reinterpret_cast<std::uintptr_t>(addresses[i]);
        if (address == 0 || address % PhysicalPageSize != 0) throw std::invalid_argument("XMem physical mapping address is null or misaligned");
        if (!requested.insert(address).second) throw std::invalid_argument("Duplicate XMem physical mapping address");
        std::optional<Page> next;
        if (frames) {
            auto it = pages.find(frames[i]);
            if (it == pages.end()) throw std::invalid_argument("XMem physical mapping frame is not allocated");
            next = it->second;
            added.emplace(address, *next);
        }
        std::optional<Page> previous;
        if (auto it = mappings.find(address); it != mappings.end()) previous = it->second;
        if (mappedPins.contains(address) && (!next || !previous || next->frame != previous->frame)) throw std::logic_error("Physical mapping has active GPU resources and cannot be changed");
        if (next && previous && next->frame == previous->frame) continue;
        changes.push_back({addresses[i], previous, next});
    }
    std::size_t completed = 0;
    try {
        for (const auto& change : changes) {
            ApplyMapping(change);
            ++completed;
        }
    } catch (...) {
        auto error = std::current_exception();
        while (completed != 0) RestoreMapping(changes[--completed]);
        std::rethrow_exception(error);
    }
    for (const auto& change : changes) {
        auto address = reinterpret_cast<std::uintptr_t>(change.address);
        if (change.next) {
            if (auto it = mappings.find(address); it != mappings.end()) it->second = *change.next;
        } else {
            mappings.erase(address);
        }
    }
    mappings.merge(added);
}
