#include "Features.hpp"

std::optional<bool> AnyBox::Relinker::GetVectorSupport(const CpuState& state, ZydisISASet isa) {
    auto basic = [&](unsigned reg, unsigned bit) { return (state.basic[reg] & (1U << bit)) != 0; };
    auto structured = [&](unsigned reg, unsigned bit) { return (state.structured[reg] & (1U << bit)) != 0; };
    auto extended = [&](unsigned reg, unsigned bit) { return (state.extended[reg] & (1U << bit)) != 0; };
    auto avx = state.HasAvx();
    switch (isa) {
    case ZYDIS_ISA_SET_SSE:
    case ZYDIS_ISA_SET_SSEMXCSR:
    case ZYDIS_ISA_SET_SSE_PREFETCH: return basic(3, 25);
    case ZYDIS_ISA_SET_SSE2: return basic(3, 26);
    case ZYDIS_ISA_SET_SSE2MMX: return basic(3, 26) && basic(3, 23);
    case ZYDIS_ISA_SET_SSE3:
    case ZYDIS_ISA_SET_SSE3X87: return basic(2, 0);
    case ZYDIS_ISA_SET_SSSE3: return basic(2, 9);
    case ZYDIS_ISA_SET_SSSE3MMX: return basic(2, 9) && basic(3, 23);
    case ZYDIS_ISA_SET_SSE4: return basic(2, 19);
    case ZYDIS_ISA_SET_SSE42: return basic(2, 20);
    case ZYDIS_ISA_SET_SSE4A: return extended(2, 6);
    case ZYDIS_ISA_SET_AES: return basic(2, 25);
    case ZYDIS_ISA_SET_PCLMULQDQ: return basic(2, 1);
    case ZYDIS_ISA_SET_SHA: return structured(1, 29);
    case ZYDIS_ISA_SET_GFNI: return structured(2, 8);
    case ZYDIS_ISA_SET_AVX_GFNI: return avx && structured(2, 8);
    case ZYDIS_ISA_SET_AVX: return avx;
    case ZYDIS_ISA_SET_AVX2:
    case ZYDIS_ISA_SET_AVX2GATHER: return avx && structured(1, 5);
    case ZYDIS_ISA_SET_AVXAES: return avx && basic(2, 25);
    case ZYDIS_ISA_SET_F16C: return avx && basic(2, 29);
    case ZYDIS_ISA_SET_FMA: return avx && basic(2, 12);
    case ZYDIS_ISA_SET_FMA4: return avx && extended(2, 16);
    case ZYDIS_ISA_SET_XOP: return avx && extended(2, 11);
    case ZYDIS_ISA_SET_VAES: return avx && structured(2, 9);
    case ZYDIS_ISA_SET_VPCLMULQDQ: return avx && structured(2, 10);
    default: return std::nullopt;
    }
}
