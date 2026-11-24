#include "ComputeFixture.hpp"

void AnyBox::Graphics::Tests::CheckComputeRoots(Device* device) {
    ComputeFixture fixture(device);
    auto* list = fixture.GetList();
    auto bind = Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33);
    auto constant = Method<decltype(&CommandList::SetComputeRoot32BitConstant)>(list, 37);
    auto constants = Method<decltype(&CommandList::SetComputeRoot32BitConstants)>(list, 39);
    auto uav = Method<decltype(&CommandList::SetComputeRootUnorderedAccessView)>(list, 45);
    auto srv = Method<decltype(&CommandList::SetComputeRootShaderResourceView)>(list, 43);
    auto cbv = Method<decltype(&CommandList::SetComputeRootConstantBufferView)>(list, 41);
    auto table = Method<decltype(&CommandList::SetComputeRootDescriptorTable)>(list, 35);
    Reject<std::logic_error>([&] { constant(list, 3, 1, 0); });
    Require(ComputeFixture::ReadHeader(list, 0) == UINT32_MAX, "Unbound compute root exposes a valid inline header");
    bind(list, fixture.GetRoot());
    for (unsigned index = 0; index < 5; ++index) Require(ComputeFixture::ReadHeader(list, index) == (0xC0000000U | (index << 10)), "Compute root inline header does not match its parameter index");
    Require(ComputeFixture::ReadHeader(list, 5) == UINT32_MAX, "Compute root exposes a header for an absent parameter");
    std::array values{11u, 12u, 13u};
    constants(list, 3, 3, values.data(), 2);
    constant(list, 3, 21, 3);
    Require(fixture.GetArguments().Read(3, 2, 3)[1] == 21, "Compute partial constant update lost its value");
    Reject<std::logic_error>([&] { fixture.GetArguments().Read(3, 0, 1); });
    bind(list, fixture.GetRoot());
    Require(fixture.GetArguments().Read(3, 2, 1)[0] == 11, "Rebinding the same root invalidated its arguments");
    uav(list, 0, 0x200001000);
    srv(list, 1, 0x200002000);
    cbv(list, 2, 0x200003000);
    table(list, 4, 0x50000000);
    Require(fixture.GetArguments().Read(0, 0, 2)[1] == 2 && fixture.GetArguments().Read(1, 0, 2)[0] == 0x2000, "Compute root descriptor did not preserve its 64-bit address");
    Require(fixture.GetArguments().Read(2, 0, 2)[0] == 0x3000 && fixture.GetArguments().Read(4, 0, 1)[0] == 0x50000000, "Compute CBV or descriptor table changed its address");
    Reject<std::invalid_argument>([&] { constants(list, 0, 1, values.data(), 0); });
    Reject<std::invalid_argument>([&] { constants(list, 3, 1, nullptr, 0); });
    Reject<std::out_of_range>([&] { constants(list, 3, 3, values.data(), 12); });
    Reject<std::invalid_argument>([&] { srv(list, 0, 0); });
    Reject<std::invalid_argument>([&] { table(list, 4, 0x100000000); });
    bind(list, nullptr);
    Require(!list->GetComputeState() && ComputeFixture::ReadHeader(list, 0) == UINT32_MAX, "Null root binding retained compute arguments");
    bind(list, fixture.GetRoot());
    Reject<std::logic_error>([&] { fixture.GetArguments().Read(3, 2, 1); });
    Require(Method<decltype(&CommandList::Close)>(list, 9)(list) == 0, "Compute root list close failed");
    Reject<std::logic_error>([&] { constant(list, 3, 1, 0); });
    Reject<std::logic_error>([&] { bind(list, nullptr); });
    Require(Method<decltype(&CommandList::Reset)>(list, 10)(list, fixture.GetAllocator(), nullptr) == 0, "Compute root list reset failed");
    Require(!list->GetComputeState() && ComputeFixture::ReadHeader(list, 0) == UINT32_MAX, "Command list reset retained its root arguments");
}
