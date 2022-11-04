#pragma once
#include "../core/egxcommon.hpp"
#include "../core/egxutil.hpp"
#include "synchronization.hpp"
#include <vector>

namespace egx
{

    class CommandBuffer
    {
    public:
        EGX_API CommandBuffer() = default;
        EGX_API CommandBuffer(const ref<VulkanCoreInterface> &CoreInterface);
        EGX_API CommandBuffer(CommandBuffer &&) noexcept;
        EGX_API ~CommandBuffer() noexcept;

        EGX_API void DelayInitialize(const ref<VulkanCoreInterface> &CoreInterface);

        EGX_API const VkCommandBuffer GetBuffer();
        EGX_API const VkCommandBuffer &GetReadonlyBuffer() const;
        EGX_API void Finalize();

        inline void SetStaticRecordIndex(uint32_t index = UINT32_MAX) { _static_record_index = index; }

        EGX_API static ref<CommandBuffer> CreateSingleBuffer(const ref<VulkanCoreInterface> &CoreInterface);

        EGX_API static void Submit(VkQueue Queue,
                                   const std::initializer_list<VkCommandBuffer> &CmdBuffers,
                                   const std::initializer_list<VkSemaphore> &WaitSemaphores,
                                   const std::initializer_list<VkPipelineStageFlags> &WaitDstStageMask,
                                   const std::initializer_list<VkSemaphore> &SignalSemaphores,
                                   VkFence SignalFence,
                                   bool Block = false);

        EGX_API static void Submit(const ref<VulkanCoreInterface> &CoreInterface,
                                   const std::initializer_list<VkCommandBuffer> &CmdBuffers,
                                   const std::initializer_list<VkSemaphore> &WaitSemaphores,
                                   const std::initializer_list<VkPipelineStageFlags> &WaitDstStageMask,
                                   const std::initializer_list<VkSemaphore> &SignalSemaphores,
                                   VkFence SignalFence,
                                   bool Block = false);

        EGX_API static void Submit(VkQueue Queue,
                                   const std::vector<VkCommandBuffer> &CmdBuffers,
                                   const std::vector<VkSemaphore> &WaitSemaphores,
                                   const std::vector<VkPipelineStageFlags> &WaitDstStageMask,
                                   const std::vector<VkSemaphore> &SignalSemaphores,
                                   VkFence SignalFence,
                                   bool Block = false);

        EGX_API static void Submit(const ref<VulkanCoreInterface> &CoreInterface,
                                   const std::vector<VkCommandBuffer> &CmdBuffers,
                                   const std::vector<VkSemaphore> &WaitSemaphores,
                                   const std::vector<VkPipelineStageFlags> &WaitDstStageMask,
                                   const std::vector<VkSemaphore> &SignalSemaphores,
                                   VkFence SignalFence,
                                   bool Block = false);

        CommandBuffer(CommandBuffer &) = delete;

        static egx::ref<CommandBuffer> FactoryCreate(const ref<VulkanCoreInterface> &CoreInterface)
        {
            return {new CommandBuffer(CoreInterface)};
        }

    private:
        inline uint32_t GetCurrentFrame() const
        {
            return _current_frame_ptr ? (_static_record_index == UINT32_MAX ? *_current_frame_ptr : _static_record_index) : 0;
        }

    private:
        std::vector<VkCommandBuffer> _cmd;
        std::vector<bool> _cmd_static_init;
        uint32_t *_current_frame_ptr = nullptr;
        uint32_t _last_frame = UINT32_MAX;
        uint32_t _static_record_index = UINT32_MAX;
    };

    class CommandBufferSingleUse
    {
    public:
        CommandBufferSingleUse(const ref<VulkanCoreInterface> &CoreInterface)
        {
            _cmd = CommandBuffer::CreateSingleBuffer(CoreInterface);
            _fence = Fence::CreateSingleFence(CoreInterface, false);
            _queue = CoreInterface->Queue;
            Cmd = _cmd->GetBuffer();
        }

        void Execute()
        {
            CommandBuffer::Submit(_queue, {Cmd}, {}, {}, {}, _fence->GetFence(), true);
        }

        CommandBufferSingleUse(CommandBufferSingleUse &) = delete;
        CommandBufferSingleUse(CommandBufferSingleUse &&) = delete;

    public:
        VkCommandBuffer Cmd;

    private:
        VkQueue _queue;
        ref<Fence> _fence;
        ref<CommandBuffer> _cmd;
    };

}