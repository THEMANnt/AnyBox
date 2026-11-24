#include "ComputeFixture.hpp"
#include <algorithm>

void AnyBox::Graphics::Tests::CheckComputeInline(Device* device) {
    ComputeFixture fixture(device);
    auto* list = fixture.GetList();
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
    auto* begin = ComputeFixture::GetWrite(list);
    std::array records{ComputeFixture::ReadHeader(list, 0), 0xABC00000u, 2u, ComputeFixture::ReadHeader(list, 3) + (2u << 16) + 8u, 7u, 8u, 9u};
    std::copy(records.begin(), records.end(), begin);
    ComputeFixture::SetWrite(list, begin + records.size());
    Method<decltype(&CommandList::SetComputeRoot32BitConstant)>(list, 37)(list, 3, 33, 3);
    Require(ComputeFixture::GetWrite(list) == begin, "Vtable root update did not consume the preceding inline records");
    Require(fixture.GetArguments().Read(0, 0, 2)[0] == 0xABC00000, "Inline root descriptor was not retained");
    Require(fixture.GetArguments().Read(3, 2, 3)[0] == 7 && fixture.GetArguments().Read(3, 2, 3)[1] == 33 && fixture.GetArguments().Read(3, 2, 3)[2] == 9, "Mixed inline and vtable root updates changed their order");
    begin[0] = ComputeFixture::ReadHeader(list, 3);
    begin[1] = 55;
    begin[2] = 0xDEAD0000;
    ComputeFixture::SetWrite(list, begin + 3);
    Reject<std::runtime_error>([&] { close(list); });
    Reject<std::logic_error>([&] { fixture.GetArguments().Read(3, 0, 1); });
    ComputeFixture::SetWrite(list, begin + 2);
    Require(close(list) == 0, "Supported inline root record could not close");
    Require(fixture.GetArguments().Read(3, 0, 1)[0] == 55, "Closing an inline root stream did not retain its state");
    Method<decltype(&CommandList::Reset)>(list, 10)(list, fixture.GetAllocator(), nullptr);
    Method<decltype(&CommandList::SetComputeRootSignature)>(list, 33)(list, fixture.GetRoot());
    begin = ComputeFixture::GetWrite(list);
    auto header = ComputeFixture::ReadHeader(list, 3);
    for (unsigned index = 0; index < 95; ++index) {
        begin[index * 2] = header;
        begin[index * 2 + 1] = index;
    }
    begin[190] = header + (13u << 16);
    for (unsigned index = 0; index < 14; ++index) begin[191 + index] = 100 + index;
    ComputeFixture::SetWrite(list, begin + 205);
    Require(close(list) == 0 && fixture.GetArguments().Read(3, 0, 14)[13] == 113, "Inline root record crossing the visible buffer boundary was not consumed safely");
}
