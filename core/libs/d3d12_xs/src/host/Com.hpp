#pragma once

#include <memory>

namespace AnyBox::Graphics {
template<class TInterface>
struct ComRelease {
    void operator()(TInterface* value) const noexcept { value->Release(); }
};

template<class TInterface>
using ComOwner = std::unique_ptr<TInterface, ComRelease<TInterface>>;
}
