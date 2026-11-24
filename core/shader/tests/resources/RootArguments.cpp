#include "Fixtures.hpp"
#include <anybox/shader/RootArguments.hpp>
#include <array>

void CheckRootArguments() {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    std::array bindings{
        BufferBinding{ResourceClass::ShaderResource, 0, 1, 7, 1, BufferKind::Structured, 32},
        BufferBinding{ResourceClass::UnorderedAccess, 0, 0, 4, 1, BufferKind::Raw},
        BufferBinding{ResourceClass::ConstantBuffer, 0, 2, 3, 1, BufferKind::Constant, 0, 52}
    };
    RootSignatureLayout root{};
    root.version = 2;
    for (auto kind : {RootParameterKind::ShaderResource, RootParameterKind::UnorderedAccess, RootParameterKind::Constants}) {
        RootParameter parameter{};
        parameter.kind = kind;
        root.parameters.push_back(parameter);
    }
    for (unsigned index = 0; index < bindings.size(); ++index) {
        root.parameters[index].space = bindings[index].space;
        root.parameters[index].shaderRegister = bindings[index].firstRegister;
    }
    root.parameters[2].wordCount = 16;
    RootArgumentPlan plan(bindings, root, 4);
    Fixtures::Require(plan.GetUserRegisterCount() == 28 && plan.GetRanges().size() == 3);
    for (unsigned index = 0; index < 3; ++index) {
        const auto& range = plan.Find(bindings[index]);
        Fixtures::Require(range.parameter == index && range.firstUserRegister == 4 + index * 4);
    }
    Fixtures::Require(plan.Find(bindings[2]).kind == RootArgumentKind::Constants);
    auto reject = [&](auto changed) { Fixtures::Reject<std::exception>([&] { RootArgumentPlan invalid(bindings, changed); }); };
    auto changed = root;
    changed.parameters.push_back(changed.parameters[0]);
    reject(changed);
    changed = root;
    changed.parameters[2].wordCount = 12;
    reject(changed);
    changed = root;
    changed.parameters[0].visibility = 1;
    reject(changed);
    changed = root;
    changed.flags = 0x80;
    reject(changed);
    changed = root;
    changed.version = 3;
    reject(changed);
    Fixtures::Reject<std::length_error>([&] { RootArgumentPlan invalid(bindings, root, 108); });
    auto missing = bindings[0];
    ++missing.space;
    Fixtures::Reject<std::invalid_argument>([&] { plan.Find(missing); });

    RootParameter table{};
    table.kind = RootParameterKind::Table;
    table.ranges = {{RootRangeKind::UnorderedAccess, 3, 10, 0, 0, 0}, {RootRangeKind::ShaderResource, 8, 5, 1, 0, UINT32_MAX}};
    root.parameters = {table};
    bindings[0].count = 3;
    auto check = [&] { return RootArgumentPlan(std::span(bindings).first(1), root); };
    auto mapped = check().GetRanges().front();
    Fixtures::Require(mapped.kind == RootArgumentKind::Table && mapped.tableOffset == 5 && mapped.wordCount == 12);
    root.parameters[0].ranges[1].count = UINT32_MAX;
    Fixtures::Require(check().GetRanges().front().tableOffset == 5);
    root.parameters[0].ranges.push_back(table.ranges[0]);
    root.parameters[0].ranges.back().tableOffset = UINT32_MAX;
    Fixtures::Reject<std::invalid_argument>(check);
    root.parameters = {table};
    root.parameters[0].ranges.push_back(table.ranges[1]);
    Fixtures::Reject<std::invalid_argument>(check);
    root.parameters = {table};
    root.parameters[0].ranges[1].tableOffset = UINT32_MAX - 1;
    Fixtures::Reject<std::invalid_argument>(check);
}
