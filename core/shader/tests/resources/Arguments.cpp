#include "Fixtures.hpp"
#include <anybox/shader/BufferArguments.hpp>
#include <array>

void CheckBufferArguments() {
    using namespace AnyBox::Shader;
    std::array bindings{
        BufferBinding{ResourceClass::ShaderResource, 0, 0, 2, 3, BufferKind::Raw},
        BufferBinding{ResourceClass::UnorderedAccess, 0, 0, 2, 1, BufferKind::Raw},
        BufferBinding{ResourceClass::ConstantBuffer, 0, 0, 0, 1, BufferKind::Constant, 0, 52}
    };
    BufferArgumentPlan plan(bindings, 8);
    Fixtures::Require(plan.GetUserRegisterCount() == 28 && plan.GetRanges().size() == 3);
    Fixtures::Require(plan.Find(bindings[0]).firstUserRegister == 8);
    Fixtures::Require(plan.Find(bindings[1]).firstUserRegister == 20);
    Fixtures::Require(plan.Find(bindings[2]).firstUserRegister == 24);
    auto different = bindings[0];
    ++different.space;
    Fixtures::Reject<std::invalid_argument>([&] { plan.Find(different); });

    std::array repeated{bindings[0], bindings[0]};
    Fixtures::Reject<std::invalid_argument>([&] { BufferArgumentPlan invalid(repeated); });
    repeated[1].rangeId = 1;
    Fixtures::Reject<std::invalid_argument>([&] { BufferArgumentPlan invalid(repeated); });
    repeated[1].space = 1;
    Fixtures::Require(BufferArgumentPlan(repeated).GetUserRegisterCount() == 24);
    auto check = [](BufferBinding binding) { return BufferArgumentPlan(std::span(&binding, 1)); };
    auto capacity = bindings[0];
    capacity.count = 32;
    Fixtures::Require(check(capacity).GetUserRegisterCount() == 128);
    capacity.count = 33;
    Fixtures::Reject<std::length_error>([&] { check(capacity); });
    capacity.count = UINT32_MAX;
    Fixtures::Reject<std::length_error>([&] { check(capacity); });
    capacity.count = 0;
    Fixtures::Reject<std::length_error>([&] { check(capacity); });
    capacity.count = 2;
    capacity.firstRegister = UINT32_MAX;
    Fixtures::Reject<std::invalid_argument>([&] { check(capacity); });
    capacity = bindings[0];
    capacity.hasCounter = true;
    Fixtures::Reject<std::runtime_error>([&] { check(capacity); });
    capacity = bindings[0];
    capacity.rasterizerOrdered = true;
    Fixtures::Reject<std::runtime_error>([&] { check(capacity); });
    capacity = bindings[0];
    capacity.kind = BufferKind::Constant;
    Fixtures::Reject<std::invalid_argument>([&] { check(capacity); });
    capacity = bindings[0];
    capacity.resourceClass = ResourceClass::Sampler;
    Fixtures::Reject<std::invalid_argument>([&] { check(capacity); });
    capacity = bindings[0];
    capacity.kind = BufferKind::Structured;
    capacity.stride = 32;
    Fixtures::Require(check(capacity).GetUserRegisterCount() == 12);
    for (unsigned stride : {0u, 3u, 2052u}) {
        capacity.stride = stride;
        Fixtures::Reject<std::invalid_argument>([&] { check(capacity); });
    }
    capacity = bindings[2];
    capacity.byteSize = 0;
    Fixtures::Reject<std::invalid_argument>([&] { check(capacity); });
    Fixtures::Reject<std::invalid_argument>([&] { BufferArgumentPlan invalid(bindings, 129); });
}
