#include <anybox/address/Context.hpp>
#include <addrinterface.h>
#include <array>
#include <stdexcept>
#include <string>

AnyBox::Address::SurfaceLayout AnyBox::Address::Context::Compute(const SurfaceDescription& description, std::span<MipLayout> mips) const {
    if (description.width == 0 || description.height == 0 || description.slices == 0 || description.samples == 0 || description.mipLevels == 0 || description.mipLevels > 16) throw std::invalid_argument("Surface dimensions, samples or mip count are invalid");
    if (!mips.empty() && mips.size() != description.mipLevels) throw std::invalid_argument("Surface mip output count does not match its description");
    if ((description.flags & 0xFFC00000) != 0 || description.resourceType > 2) throw std::invalid_argument("Surface flags or resource type are invalid");
    ADDR2_COMPUTE_SURFACE_INFO_INPUT input{};
    input.size = sizeof(input);
    input.width = description.width;
    input.height = description.height;
    input.numSlices = description.slices;
    input.numMipLevels = description.mipLevels;
    input.numSamples = description.samples;
    input.numFrags = description.fragments;
    input.bpp = description.bitsPerElement;
    input.format = static_cast<AddrFormat>(description.format);
    input.swizzleMode = static_cast<AddrSwizzleMode>(description.swizzleMode);
    input.resourceType = static_cast<AddrResourceType>(description.resourceType);
    input.flags.value = description.flags;
    input.pitchInElement = description.pitch;
    input.sliceAlign = description.sliceAlignment;
    std::array<ADDR2_MIP_INFO, 16> information{};
    ADDR2_COMPUTE_SURFACE_INFO_OUTPUT output{};
    output.size = sizeof(output);
    if (!mips.empty()) output.pMipInfo = information.data();
    auto result = Addr2ComputeSurfaceInfo(handle, &input, &output);
    if (result != ADDR_OK) throw std::runtime_error("AddrLib surface calculation failed: " + std::to_string(result));
    if (output.surfSize == 0 || output.baseAlign == 0 || (output.baseAlign & (output.baseAlign - 1)) != 0) throw std::runtime_error("AddrLib returned invalid surface allocation bounds");
    for (std::size_t index = 0; index < mips.size(); ++index) {
        const auto& mip = information[index];
        mips[index] = {mip.pitch, mip.height, mip.depth, mip.pixelPitch, mip.pixelHeight, mip.equationIndex, mip.offset, mip.macroBlockOffset, mip.mipTailOffset, mip.mipTailCoordX, mip.mipTailCoordY, mip.mipTailCoordZ};
        if (description.mipLevels == 1) {
            mips[index].pixelPitch = output.pixelPitch;
            mips[index].pixelHeight = output.pixelHeight;
        } else if (description.format == 0) {
            mips[index].pixelPitch = mip.pitch;
            mips[index].pixelHeight = mip.height;
        }
    }
    return {output.surfSize, output.sliceSize, output.baseAlign, output.pitch, output.height, output.numSlices, output.pixelPitch, output.pixelHeight, output.bpp, output.blockWidth, output.blockHeight, output.blockSlices, output.firstMipIdInTail, output.mipChainInTail != 0};
}
