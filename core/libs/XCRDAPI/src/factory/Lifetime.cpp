#include "Factory.hpp"
#include "../file/File.hpp"

#include <stdexcept>

std::int32_t AnyBox::Storage::Factory::QueryInterface(const Guid& interfaceId, void** object) {
    if (!object) throw std::invalid_argument("DirectStorage factory output is null");
    if (interfaceId != FactoryId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage factory interface");
    AddRef();
    *object = static_cast<IStorageFactory*>(this);
    return 0;
}

std::uint32_t AnyBox::Storage::Factory::AddRef() {
    return references.Add();
}

std::uint32_t AnyBox::Storage::Factory::Release() {
    return references.Release();
}

std::int32_t AnyBox::Storage::Factory::OpenFile(const wchar_t* path, const Guid& interfaceId, void** file) {
    if (!file) throw std::invalid_argument("DirectStorage file output is null");
    if (interfaceId != FileId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage file interface");
    auto* result = new File(path);
    *file = static_cast<IStorageFile*>(result);
    return 0;
}

AnyBox::Storage::Factory& AnyBox::Storage::GetFactory() {
    static Factory factory;
    return factory;
}
