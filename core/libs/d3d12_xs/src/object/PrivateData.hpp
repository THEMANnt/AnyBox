#pragma once

#include "Properties.hpp"
#include <array>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace AnyBox::Graphics {
class PrivateData {
public:
    HRESULT Get(const Guid& key, std::uint32_t* size, void* data) const;
    HRESULT Set(const Guid& key, std::uint32_t size, const void* data);
    HRESULT SetInterface(const Guid& key, const IUnknown* object);

private:
    struct Value {
        ~Value();
        std::vector<std::byte> bytes;
        IUnknown* object = nullptr;
    };
    using Key = std::array<std::byte, sizeof(Guid)>;
    static Key makeKey(const Guid& key);
    HRESULT replace(const Guid& key, std::shared_ptr<Value> value);
    mutable std::mutex mutex;
    std::map<Key, std::shared_ptr<Value>> values;
};
}
