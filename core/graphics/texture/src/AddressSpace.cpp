#include <anybox/texture/AddressSpace.hpp>
#include <exception>
#include <stdexcept>
#include <map>
#include <mutex>

struct AnyBox::Texture::AddressSpace::State {
    mutable std::mutex mutex;
    std::map<std::uint64_t, Resource*> resources;
};

AnyBox::Texture::AddressSpace::AddressSpace() : state(new State) {}

AnyBox::Texture::AddressSpace::~AddressSpace() {
    if (!state->resources.empty()) std::terminate();
    delete state;
}

void AnyBox::Texture::AddressSpace::insert(std::uint64_t address, Resource& resource) {
    std::scoped_lock lock(state->mutex);
    if (!state->resources.emplace(address, &resource).second) throw std::logic_error("Texture GPU address is already registered");
}

void AnyBox::Texture::AddressSpace::remove(std::uint64_t address) noexcept {
    std::scoped_lock lock(state->mutex);
    if (state->resources.erase(address) != 1) std::terminate();
}

AnyBox::Texture::Resource& AnyBox::Texture::AddressSpace::Resolve(std::uint64_t address) const {
    std::scoped_lock lock(state->mutex);
    auto found = state->resources.find(address);
    if (found == state->resources.end()) throw std::out_of_range("Texture GPU address is not registered");
    return *found->second;
}
