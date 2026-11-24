#include "../../src/native/FirstLane.hpp"
#include "NativeFixtures.hpp"
#include "MaskLoopFixture.hpp"

namespace {
using namespace NativeMaskTests;

void Check(std::uint32_t waveSize, Mode mode, bool expected, bool bitWords = false) {
    Fixture fixture(waveSize, mode, bitWords);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(fixture.program) == (expected ? 1 : 0));
    Fixtures::Require(fixture.read->Argument(0) == (expected ? fixture.payload : fixture.value) && fixture.read->Argument(1) == fixture.mask);
    ValidateProgram(fixture.program, true);
}
}

void CheckBallotLoopGuards() {
    for (std::uint32_t waveSize : {32u, 64u}) {
        for (bool bitWords : {false, true}) {
            Check(waveSize, Mode::Narrow, true, bitWords);
            for (auto mode : {Mode::Widen, Mode::UnguardedWords, Mode::UnguardedBoolean, Mode::Changing}) Check(waveSize, mode, false, bitWords);
        }
    }
    Check(64, Mode::SwappedHalves, false);
    Check(64, Mode::WrongPartition, false);
    Check(64, Mode::SwappedHalves, false, true);
    Check(64, Mode::WrongPartition, false, true);
}
