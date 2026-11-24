#pragma once

#include "../resource/Resource.hpp"

namespace AnyBox::Graphics {
struct RenderTargetView {
    const ObjectTable* vtable;
    std::uint32_t format;
    std::uint32_t flags;
    Resource* resource;
    Descriptor::ColorTargetWords registers;
};

const ObjectTable& GetRenderTargetTable();
static_assert(sizeof(RenderTargetView) == 72);
static_assert(offsetof(RenderTargetView, resource) == 16);
static_assert(offsetof(RenderTargetView, registers) == 24);
}
