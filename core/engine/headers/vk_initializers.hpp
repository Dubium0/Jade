#pragma once
#include <string>

#include "vk_types.hpp"

namespace vkInit{
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex,VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = VkSemaphoreCreateFlags{} );

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

    VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

    VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
        VkSemaphoreSubmitInfo* waitSemaphoreInfo);

    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    
    VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

    VkRenderingAttachmentInfo attachmentInfo(
        VkImageView view,  VkClearValue* clear ,VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment,
        VkRenderingAttachmentInfo* depthAttachment);
    VkPipelineShaderStageCreateInfo pipelineShaderStagecreateInfo(VkShaderStageFlagBits stage,
            VkShaderModule shaderModule,
            const std::string&  entryName);
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();
    VkRenderingAttachmentInfo depthAttachmentInfo(
        VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}