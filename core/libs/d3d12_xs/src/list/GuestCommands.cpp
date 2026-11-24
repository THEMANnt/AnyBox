#include "List.hpp"
#include "Compute.hpp"
#include <anybox/graphics/commands/Inline.hpp>
#include <stdexcept>

void AnyBox::Graphics::CommandList::resetGuestCommands() noexcept {
    guest.commandWrite = inlineCommands.data();
    guest.dispatchEnd = inlineCommands.data() + inlineCommands.size() - commandSlack;
    guest.commandEnd = guest.dispatchEnd;
    std::uint32_t clean = 0;
    std::memcpy(guest.driverState.data() + 320 - 56, &clean, sizeof(clean));
    updateRootHeaders();
}

void AnyBox::Graphics::CommandList::validateGuestCommands() const {
    auto begin = reinterpret_cast<std::uintptr_t>(inlineCommands.data());
    auto end = begin + sizeof(inlineCommands);
    auto visibleEnd = end - commandSlack * sizeof(std::uint32_t);
    auto write = reinterpret_cast<std::uintptr_t>(guest.commandWrite);
    if (write < begin || write > end || (write - begin) % sizeof(std::uint32_t) != 0 || reinterpret_cast<std::uintptr_t>(guest.dispatchEnd) != visibleEnd || reinterpret_cast<std::uintptr_t>(guest.commandEnd) != visibleEnd) throw std::logic_error("Xbox command stream bounds are corrupted");
}

void AnyBox::Graphics::CommandList::updateRootHeaders() noexcept {
    auto count = compute ? compute->GetRoot()->GetLayout().parameters.size() : 0;
    for (std::uint32_t index = 0; index < 64; ++index) {
        auto header = index < count ? Commands::RootHeader(index) : UINT32_MAX;
        std::memcpy(guest.driverState.data() + 0x344 - 56 + index * 4, &header, 4);
    }
}

void AnyBox::Graphics::CommandList::flushGuestCommands() {
    validateGuestCommands();
    if (guest.commandWrite == inlineCommands.data()) return;
    if (!compute) throw std::runtime_error("Xbox inline command stream has no bound compute root signature");
    auto words = std::span(inlineCommands.data(), static_cast<std::size_t>(guest.commandWrite - inlineCommands.data()));
    Commands::InlineReader validation(words);
    while (!validation.Empty()) {
        if (std::holds_alternative<Commands::Dispatch>(validation.Next(compute->GetArguments()))) throw std::runtime_error("Xbox inline compute dispatch execution is not implemented");
    }
    Commands::InlineReader reader(words);
    while (!reader.Empty()) {
        auto update = std::get<Commands::RootWrite>(reader.Next(compute->GetArguments()));
        compute->GetArguments().Write(update.parameter, update.offset, update.words);
    }
    guest.commandWrite = inlineCommands.data();
}
