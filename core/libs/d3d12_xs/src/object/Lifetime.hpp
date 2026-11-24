#pragma once

#include "Properties.hpp"
#include <anybox/Types.hpp>
#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace AnyBox::Graphics {
template<class TObject, template<class> class TProperties = ObjectProperties>
class ObjectLifetime : public TProperties<TObject> {
public:
    static std::int32_t QueryInterface(TObject* self, const Guid& interfaceId, void** object) {
        if (!object) throw std::invalid_argument("Graphics interface output is null");
        *object = nullptr;
        if (!self->Supports(interfaceId)) throw std::invalid_argument("Unsupported graphics object interface");
        AddRef(self);
        *object = self;
        return 0;
    }

    static std::uint32_t AddRef(TObject* self) {
        auto& references = self->getReferences();
        auto previous = references.load(std::memory_order_relaxed);
        do {
            if (previous == 0 || previous == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Graphics object reference count is invalid");
        } while (!references.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
        return previous + 1;
    }

    static std::uint32_t Release(TObject* self) {
        auto previous = self->getReferences().fetch_sub(1, std::memory_order_acq_rel);
        if (previous == 0) throw std::logic_error("Graphics object reference count underflows");
        if (previous == 1) delete self;
        return previous - 1;
    }

    static std::int32_t GetDevice(TObject* self, const Guid& interfaceId, void** device) { return self->device->QueryInterface(self->device, interfaceId, device); }
};
}
