#pragma once

#include <cstdint>

namespace AnyBox::Texture {
class Resource;

class AddressSpace {
public:
    AddressSpace();
    ~AddressSpace();
    AddressSpace(const AddressSpace&) = delete;
    AddressSpace& operator=(const AddressSpace&) = delete;
    Resource& Resolve(std::uint64_t address) const;

private:
    friend class Resource;
    void insert(std::uint64_t address, Resource& resource);
    void remove(std::uint64_t address) noexcept;
    struct State;
    State* state;
};
}
