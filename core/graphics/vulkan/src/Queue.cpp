#include <anybox/vulkan/Timeline.hpp>

void AnyBox::Vulkan::Context::Submit(std::uint32_t queueIndex, const VkSubmitInfo& submission, VkFence fence) {
    auto queue = GetQueue(queueIndex);
    std::scoped_lock lock(queueMutexes[queueIndex]);
    Check(deviceFunctions.queueSubmit(queue, 1, &submission, fence), "Submit Vulkan queue commands");
}

void AnyBox::Vulkan::Timeline::SignalOnQueue(std::uint32_t queueIndex, std::uint64_t value) {
    if (!semaphore) throw std::logic_error("Vulkan timeline semaphore is empty");
    VkTimelineSemaphoreSubmitInfo timeline{};
    timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline.signalSemaphoreValueCount = 1;
    timeline.pSignalSemaphoreValues = &value;
    VkSubmitInfo submission{};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission.pNext = &timeline;
    submission.signalSemaphoreCount = 1;
    submission.pSignalSemaphores = &semaphore;
    context.Submit(queueIndex, submission);
}

void AnyBox::Vulkan::Timeline::WaitOnQueue(std::uint32_t queueIndex, std::uint64_t value) {
    if (!semaphore) throw std::logic_error("Vulkan timeline semaphore is empty");
    VkTimelineSemaphoreSubmitInfo timeline{};
    timeline.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline.waitSemaphoreValueCount = 1;
    timeline.pWaitSemaphoreValues = &value;
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkSubmitInfo submission{};
    submission.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submission.pNext = &timeline;
    submission.waitSemaphoreCount = 1;
    submission.pWaitSemaphores = &semaphore;
    submission.pWaitDstStageMask = &stage;
    context.Submit(queueIndex, submission);
}
