#pragma once
#include <vulkan/vulkan.h>
#include <vector>
namespace vkUtil{
bool loadShaderModule(const std::string& filePath,
    VkDevice device,
    VkShaderModule& outShaderModule);
  
}

class PipelineBuilder {
    public:
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};
       
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineMultisampleStateCreateInfo multisampling{};
        VkPipelineLayout pipelineLayout{};
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        VkPipelineRenderingCreateInfo renderInfo{};
        VkFormat colorAttachmentformat{};
    
        PipelineBuilder(){ clear(); }
    
        void clear();
        
        VkPipeline buildPipeline(VkDevice device);
        void setShaders(VkShaderModule vertexShader, const std::string& vertexShaderEntryName, 
            VkShaderModule fragmentShader,const std::string& fragmentShaderEntryName);
        void setInputTopology(VkPrimitiveTopology topology);        
        void setPolygonMode(VkPolygonMode mode);
        void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
        void setMultisamplingNone();
        void disableBlending();
        void setColorAttachmentFormat(VkFormat format);
        void setDepthFormat(VkFormat format);
        void disableDepthTest();
        void enableDepthTest(bool depthWriteEnable, VkCompareOp compOp);
        void enableBlendingAdditive();
        void enableBlendingAlphablend();
};