#include "Features.hpp"

std::optional<bool> AnyBox::Relinker::GetScalarSupport(const CpuState& state, ZydisISASet isa) {
    auto basic = [&](unsigned reg, unsigned bit) { return (state.basic[reg] & (1U << bit)) != 0; };
    auto structured = [&](unsigned reg, unsigned bit) { return (state.structured[reg] & (1U << bit)) != 0; };
    auto extended = [&](unsigned reg, unsigned bit) { return (state.extended[reg] & (1U << bit)) != 0; };
    switch (isa) {
    case ZYDIS_ISA_SET_I86:
    case ZYDIS_ISA_SET_I186:
    case ZYDIS_ISA_SET_I286REAL:
    case ZYDIS_ISA_SET_I286PROTECTED:
    case ZYDIS_ISA_SET_I386:
    case ZYDIS_ISA_SET_I486:
    case ZYDIS_ISA_SET_I486REAL:
    case ZYDIS_ISA_SET_LONGMODE:
    case ZYDIS_ISA_SET_PENTIUMREAL:
    case ZYDIS_ISA_SET_PPRO:
    case ZYDIS_ISA_SET_FAT_NOP:
    case ZYDIS_ISA_SET_PAUSE: return true;
    case ZYDIS_ISA_SET_X87: return basic(3, 0);
    case ZYDIS_ISA_SET_PENTIUMMMX: return basic(3, 23);
    case ZYDIS_ISA_SET_CMOV:
    case ZYDIS_ISA_SET_FCMOV:
    case ZYDIS_ISA_SET_FCOMI: return basic(3, 15);
    case ZYDIS_ISA_SET_CMPXCHG16B: return basic(2, 13);
    case ZYDIS_ISA_SET_CLFSH: return basic(3, 19);
    case ZYDIS_ISA_SET_FXSAVE:
    case ZYDIS_ISA_SET_FXSAVE64: return basic(3, 24);
    case ZYDIS_ISA_SET_LAHF: return extended(2, 0);
    case ZYDIS_ISA_SET_LZCNT: return extended(2, 5);
    case ZYDIS_ISA_SET_POPCNT: return basic(2, 23);
    case ZYDIS_ISA_SET_MOVBE: return basic(2, 22);
    case ZYDIS_ISA_SET_RDRAND: return basic(2, 30);
    case ZYDIS_ISA_SET_RDSEED: return structured(1, 18);
    case ZYDIS_ISA_SET_RDPID: return structured(2, 22);
    case ZYDIS_ISA_SET_RDTSCP: return extended(3, 27);
    case ZYDIS_ISA_SET_BMI1: return structured(1, 3);
    case ZYDIS_ISA_SET_BMI2: return structured(1, 8);
    case ZYDIS_ISA_SET_ADOX_ADCX: return structured(1, 19);
    case ZYDIS_ISA_SET_CLFLUSHOPT: return structured(1, 23);
    case ZYDIS_ISA_SET_CLWB: return structured(1, 24);
    case ZYDIS_ISA_SET_SERIALIZE: return structured(3, 14);
    case ZYDIS_ISA_SET_RTM: return structured(1, 11);
    case ZYDIS_ISA_SET_MONITORX: return extended(2, 29);
    case ZYDIS_ISA_SET_PREFETCH_NOP: return extended(2, 8);
    case ZYDIS_ISA_SET_XSAVE: return basic(2, 26) && basic(2, 27);
    case ZYDIS_ISA_SET_XSAVEOPT: return basic(2, 27) && (state.save[0] & 1) != 0;
    case ZYDIS_ISA_SET_XSAVEC: return basic(2, 27) && (state.save[0] & 2) != 0;
    default: return std::nullopt;
    }
}
