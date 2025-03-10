#pragma once

#include <vector>
#include <span>
#include <deque>

#include <vulkan/vulkan.h>

struct DescriptorLayoutBuilder {

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
};


struct DescriptorAllocator {

    struct PoolSizeRatio{
      VkDescriptorType type;
      float ratio;
    };

    VkDescriptorPool pool;

    void initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clearDescriptors(VkDevice device);
    void destroyPool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};

class DescriptorAllocatorGrowable {
  public:
    struct PoolSizeRatio {
      VkDescriptorType type;
      float ratio;
    };
  
    void init(VkDevice t_device, uint32_t t_maxSets, std::span<PoolSizeRatio> t_poolRatios);
    void clearPools(VkDevice t_device);
    void destroyPools(VkDevice t_device);
  
    VkDescriptorSet allocate(VkDevice t_device, VkDescriptorSetLayout t_layout, void* t_pNext = nullptr);
  private:
    VkDescriptorPool getPool(VkDevice t_device);
    VkDescriptorPool createPool(VkDevice t_device, uint32_t t_setCount, std::span<PoolSizeRatio> t_poolRatios);
  
    std::vector<PoolSizeRatio> m_ratios;
    std::vector<VkDescriptorPool> m_fullPools;
    std::vector<VkDescriptorPool> m_readyPools;
    uint32_t m_setsPerPool = 0;
  
};


class DescriptorWriter {
  public:
    std::deque<VkDescriptorImageInfo> g_imageInfos;
    std::deque<VkDescriptorBufferInfo> g_bufferInfos;
    std::vector<VkWriteDescriptorSet> g_writes;

    void writeImage(int t_binding,VkImageView t_image,VkSampler t_sampler , VkImageLayout t_layout,
       VkDescriptorType t_type);
    void writeBuffer(int t_binding,VkBuffer t_buffer,size_t t_size, size_t t_offset,VkDescriptorType t_type); 

    void clear();
    void updateSet(VkDevice t_device, VkDescriptorSet t_set);
};