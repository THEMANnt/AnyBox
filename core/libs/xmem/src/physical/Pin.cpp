#include "Lease.hpp"

#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>

AnyBox::IPhysicalPageLease* AnyBox::XMem::Pool::Acquire(std::span<const std::uint64_t> frames) {
    if (frames.empty()) throw std::invalid_argument("Physical page lease requires a nonempty page array");
    std::lock_guard lock(mutex);
    std::vector<Page> retained;
    retained.reserve(frames.size());
    for (auto frame : frames) {
        auto page = pages.find(frame);
        if (page == pages.end()) throw std::invalid_argument("Physical page lease refers to an unallocated frame");
        retained.push_back(page->second);
    }
    return retain(std::move(retained), 0);
}

AnyBox::IPhysicalPageLease* AnyBox::XMem::Pool::retain(std::vector<Page> retained, std::uintptr_t mappingBase) {
    std::map<std::uint64_t, std::size_t> increments;
    std::map<std::uint64_t, std::size_t> added;
    std::map<std::uintptr_t, std::size_t> addedMappings;
    for (const auto& page : retained) ++increments[page.frame];
    for (const auto& [frame, count] : increments) {
        auto existing = pinned.find(frame);
        if (existing == pinned.end()) added.emplace(frame, 0);
        else if (count > std::numeric_limits<std::size_t>::max() - existing->second) throw std::overflow_error("Physical page pin count overflows");
    }
    if (mappingBase != 0) {
        for (std::size_t index = 0; index < retained.size(); ++index) {
            auto address = mappingBase + index * PhysicalPageSize;
            auto existing = mappedPins.find(address);
            if (existing == mappedPins.end()) addedMappings.emplace(address, 0);
            else if (existing->second == std::numeric_limits<std::size_t>::max()) throw std::overflow_error("Physical mapping pin count overflows");
        }
    }
    auto totalPages = retained.size();
    auto lease = std::make_unique<PhysicalPageLease>(*this, std::move(retained), mappingBase);
    pinned.merge(added);
    mappedPins.merge(addedMappings);
    for (const auto& [frame, count] : increments) pinned.at(frame) += count;
    if (mappingBase != 0) for (std::size_t index = 0; index < totalPages; ++index) ++mappedPins.at(mappingBase + index * PhysicalPageSize);
    lease->Activate();
    return lease.release();
}

void AnyBox::XMem::Pool::Release(std::span<const Page> retained, std::uintptr_t mappingBase) noexcept {
    std::lock_guard lock(mutex);
    for (const auto& page : retained) {
        auto existing = pinned.find(page.frame);
        if (existing == pinned.end() || existing->second == 0) std::terminate();
        if (--existing->second == 0) pinned.erase(existing);
    }
    if (mappingBase != 0) {
        for (std::size_t index = 0; index < retained.size(); ++index) {
            auto existing = mappedPins.find(mappingBase + index * PhysicalPageSize);
            if (existing == mappedPins.end() || existing->second == 0) std::terminate();
            if (--existing->second == 0) mappedPins.erase(existing);
        }
    }
}

AnyBox::IPhysicalPageLease* AnyBoxAcquirePhysicalPages(std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false) {
    if (pageCount == 0 || !pageFrames) throw std::invalid_argument("Physical page lease requires a count and array");
    return AnyBox::XMem::GetPool().Acquire({pageFrames, pageCount});
}
