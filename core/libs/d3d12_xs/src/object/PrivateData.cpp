#include "PrivateData.hpp"
#include <dxgi.h>
#include <cstring>

AnyBox::Graphics::PrivateData::Value::~Value() { if (object) object->Release(); }

AnyBox::Graphics::PrivateData::Key AnyBox::Graphics::PrivateData::makeKey(const Guid& key) {
    Key result;
    static_assert(std::has_unique_object_representations_v<Guid>);
    std::memcpy(result.data(), &key, sizeof(key));
    return result;
}

HRESULT AnyBox::Graphics::PrivateData::Get(const Guid& key, std::uint32_t* size, void* data) const {
    if (!size) throw std::invalid_argument("Graphics private data size output is null");
    std::shared_ptr<Value> value;
    {
        std::scoped_lock lock(mutex);
        auto found = values.find(makeKey(key));
        if (found != values.end()) value = found->second;
    }
    if (!value) { *size = 0; return DXGI_ERROR_NOT_FOUND; }
    auto capacity = *size;
    auto required = value->object ? sizeof(value->object) : value->bytes.size();
    *size = static_cast<std::uint32_t>(required);
    if (!data) return S_OK;
    if (capacity < required) return DXGI_ERROR_MORE_DATA;
    if (value->object) {
        value->object->AddRef();
        std::memcpy(data, &value->object, required);
    } else if (required != 0) std::memcpy(data, value->bytes.data(), required);
    return S_OK;
}

HRESULT AnyBox::Graphics::PrivateData::replace(const Guid& key, std::shared_ptr<Value> value) {
    std::shared_ptr<Value> previous;
    {
        std::scoped_lock lock(mutex);
        auto stored = makeKey(key);
        if (value) {
            auto found = values.try_emplace(stored).first;
            previous = std::move(found->second);
            found->second = std::move(value);
        } else {
            auto found = values.find(stored);
            if (found == values.end()) return S_FALSE;
            previous = std::move(found->second);
            values.erase(found);
        }
    }
    return S_OK;
}

HRESULT AnyBox::Graphics::PrivateData::Set(const Guid& key, std::uint32_t size, const void* data) {
    if (!data) {
        if (size != 0) throw std::invalid_argument("Graphics private data is null");
        return replace(key, {});
    }
    auto value = std::make_shared<Value>();
    value->bytes.resize(size);
    if (size != 0) std::memcpy(value->bytes.data(), data, size);
    return replace(key, std::move(value));
}

HRESULT AnyBox::Graphics::PrivateData::SetInterface(const Guid& key, const IUnknown* object) {
    auto value = std::make_shared<Value>();
    value->object = const_cast<IUnknown*>(object);
    if (object) value->object->AddRef();
    else {
        value->bytes.resize(sizeof(value->object));
        std::memcpy(value->bytes.data(), &value->object, sizeof(value->object));
    }
    return replace(key, std::move(value));
}
