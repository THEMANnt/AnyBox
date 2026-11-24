#include <anybox/relinker/Decoder.hpp>
#include <stdexcept>

AnyBox::Relinker::Decoder::Decoder() {
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64))) throw std::runtime_error("Cannot initialize the AMD64 decoder");
}

AnyBox::Relinker::DecodedInstruction AnyBox::Relinker::Decoder::Decode(std::span<const std::byte> code) const {
    DecodedInstruction result;
    if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, code.data(), code.size(), &result.native, result.operands.data()))) throw std::runtime_error("Cannot decode the AMD64 instruction");
    if (result.native.length == 0 || result.native.length > code.size()) throw std::runtime_error("Decoded AMD64 instruction length is invalid");
    return result;
}
