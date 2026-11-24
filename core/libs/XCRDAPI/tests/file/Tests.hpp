#ifndef ANYBOX_STORAGE_FILE_TESTS_HPP
#define ANYBOX_STORAGE_FILE_TESTS_HPP

#include <XCRDAPI.hpp>
#include <StorageInterfaces.hpp>
#include <anybox/platform/WindowsError.hpp>

#include <memory>
#include <stdexcept>

namespace AnyBox::Tests {

inline void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void ExpectException(TCallable&& callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected DirectStorage error was not raised");
}

struct ReleaseObject {
    void operator()(IStorageUnknown* object) const { object->Release(); }
};

template<class TObject>
using Object = std::unique_ptr<TObject, ReleaseObject>;

inline Object<IStorageFactory> GetFactory() {
    void* factory = nullptr;
    DStorageGetFactory(Storage::FactoryId, &factory);
    return Object<IStorageFactory>(static_cast<IStorageFactory*>(factory));
}

void CheckFactory();
void CheckFiles();

}

#endif
