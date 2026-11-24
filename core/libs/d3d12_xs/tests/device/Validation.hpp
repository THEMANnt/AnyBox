#pragma once

#include "Tests.hpp"
#include <d3d12sdklayers.h>

namespace AnyBox::Graphics::Tests {
class NativeValidation {
public:
    NativeValidation();
    ~NativeValidation();
    NativeValidation(const NativeValidation&) = delete;
    NativeValidation& operator=(const NativeValidation&) = delete;

private:
    HMODULE module;
    ID3D12Debug* debug;
};

void CheckValidation(Device* device);
}
