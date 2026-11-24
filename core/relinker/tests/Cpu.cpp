#include "Tests.hpp"
#include <anybox/relinker/Cpu.hpp>
#include <initializer_list>

namespace {
bool Supports(const AnyBox::Relinker::CpuState& state, std::initializer_list<std::uint8_t> bytes) {
    AnyBox::Relinker::Decoder decoder;
    return AnyBox::Relinker::Cpu(state).Supports(decoder.Decode(std::as_bytes(std::span(bytes.begin(), bytes.size()))));
}
}

void AnyBox::Relinker::Tests::CheckCpu() {
    CpuState state;
    state.basic[2] = (1U << 26) | (1U << 27) | (1U << 28);
    state.structured[1] = 1U << 5;
    Require(!Supports(state, {0xC5, 0xFC, 0x57, 0xC0}), "AVX accepted without OS register state");
    state.enabledState = 2;
    Require(!Supports(state, {0xC5, 0xFC, 0x57, 0xC0}), "AVX accepted without YMM state");
    state.enabledState = 6;
    Require(Supports(state, {0xC5, 0xFC, 0x57, 0xC0}), "AVX rejected with CPU and OS support");
    Require(Supports(state, {0xC5, 0xFD, 0xFE, 0xC0}), "AVX2 rejected with CPU and OS support");
    state.basic[2] &= ~(1U << 27);
    Require(!Supports(state, {0xC5, 0xFD, 0xFE, 0xC0}), "AVX2 accepted without OSXSAVE");
    state.basic[2] |= 1U << 27;
    state.structured[1] = 0;
    Require(!Supports(state, {0xC5, 0xFD, 0xFE, 0xC0}), "AVX was mistaken for AVX2");

    Require(!Supports(state, {0x66, 0x0F, 0xAE, 0x30}), "CLWB accepted without its CPUID bit");
    state.structured[1] = 1U << 24;
    Require(Supports(state, {0x66, 0x0F, 0xAE, 0x30}), "CLWB rejected with its CPUID bit");
    Require(!Supports(state, {0x0F, 0xAE, 0x38}), "CLFLUSH accepted without its CPUID bit");
    state.basic[3] = 1U << 19;
    Require(Supports(state, {0x0F, 0xAE, 0x38}), "CLFLUSH rejected with its CPUID bit");
    Require(!Supports(state, {0x0F, 0x32}), "Privileged RDMSR was accepted for title execution");
    Require(!Supports(state, {0xF3, 0x0F, 0x01, 0xEC}), "Unvalidated UINTR was accepted");
    Require(Supports(state, {0xF3, 0x0F, 0x1E, 0xFA}), "ENDBR64 NOP compatibility was lost");
    Require(!Supports(state, {0x0F, 0x01, 0xFA}) && !Supports(state, {0x0F, 0x01, 0xFB}), "MONITORX/MWAITX accepted without their CPUID bit");
    state.extended[2] |= 1U << 29;
    Require(Supports(state, {0x0F, 0x01, 0xFA}) && Supports(state, {0x0F, 0x01, 0xFB}), "Native MONITORX/MWAITX support was not recognized");
    Cpu host(CpuState::Detect());
    Require(host.Supports(Decoder().Decode(std::as_bytes(std::span("\xC3", 1)))), "Host rejected baseline RET");
}
