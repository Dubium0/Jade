#include "vk_descriptors.hpp"
#include "vk_types.hpp"
void DescriptorLayoutBuilder::addBinding(uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind {};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}
VkDescriptorSetLayout DescriptorLayoutBuilder::build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext, VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : bindings) {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = (uint32_t)bindings.size();
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

void DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * maxSets)
        });
    }

	VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = maxSets;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
}

void DescriptorAllocator::clearDescriptors(VkDevice device)
{
    vkResetDescriptorPool(device, pool, 0);
}

void DescriptorAllocator::destroyPool(VkDevice device)
{
    vkDestroyDescriptorPool(device,pool,nullptr);
}

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

    return ds;
}

void DescriptorAllocatorGrowable::init(VkDevice t_device, uint32_t t_maxSets, std::span<PoolSizeRatio> t_poolRatios){
    m_ratios.clear();
    
    for (auto ratio : t_poolRatios) {
        m_ratios.push_back(ratio);
    }
	
    VkDescriptorPool newPool = createPool(t_device, t_maxSets, t_poolRatios);

    m_setsPerPool = t_maxSets * 1.5; //grow it next allocation

    m_readyPools.push_back(newPool);
}

void DescriptorAllocatorGrowable::clearPools(VkDevice t_device){
    for (auto pool : m_readyPools) {
        vkResetDescriptorPool(t_device, pool, 0);
    }
    for (auto pool : m_fullPools) {
        vkResetDescriptorPool(t_device, pool, 0);
        m_readyPools.push_back(pool);
    }
    m_fullPools.clear();
}

void DescriptorAllocatorGrowable::destroyPools(VkDevice t_device){
    for (auto pool : m_readyPools) {
		vkDestroyDescriptorPool(t_device, pool, nullptr);
	}
    m_readyPools.clear();
	for (auto pool : m_fullPools) {
		vkDestroyDescriptorPool(t_device, pool,nullptr);
    }
    m_fullPools.clear();
}

VkDescriptorSet DescriptorAllocatorGrowable::allocate(VkDevice t_device, VkDescriptorSetLayout t_layout, void* t_pNext){
    //get or create a pool to allocate from
    VkDescriptorPool poolToUse = getPool(t_device);

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.pNext = t_pNext;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = poolToUse;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &t_layout;

	VkDescriptorSet descriptorSet;
	VkResult result = vkAllocateDescriptorSets(t_device, &allocInfo, &descriptorSet);

    //allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

        m_fullPools.push_back(poolToUse);
    
        poolToUse = getPool(t_device);
        allocInfo.descriptorPool = poolToUse;

       VK_CHECK( vkAllocateDescriptorSets(t_device, &allocInfo, &descriptorSet));
    }
  
    m_readyPools.push_back(poolToUse);
    return descriptorSet;
}

VkDescriptorPool DescriptorAllocatorGrowable::getPool(VkDevice t_device){
    VkDescriptorPool newPool;
    if (m_readyPools.size() != 0) {
        newPool = m_readyPools.back();
        m_readyPools.pop_back();
    }
    else {
	    //need to create a new pool
	    newPool = createPool(t_device, m_setsPerPool, m_ratios);

	    m_setsPerPool = m_setsPerPool * 1.5;
	    if (m_setsPerPool > 4092) {
		    m_setsPerPool = 4092;
	    }
    }   

    return newPool;
}

VkDescriptorPool DescriptorAllocatorGrowable::createPool(VkDevice t_device, uint32_t t_setCount, std::span<PoolSizeRatio> t_poolRatios){
    
    std::vector<VkDescriptorPoolSize> poolSizes;
	for (PoolSizeRatio ratio : t_poolRatios) {
		poolSizes.push_back(VkDescriptorPoolSize{
			.type = ratio.type,
			.descriptorCount = uint32_t(ratio.ratio * t_setCount)
		});
	}

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = 0;
	pool_info.maxSets = t_setCount;
	pool_info.poolSizeCount = (uint32_t)poolSizes.size();
	pool_info.pPoolSizes = poolSizes.data();

	VkDescriptorPool newPool;
	vkCreateDescriptorPool(t_device, &pool_info, nullptr, &newPool);
    return newPool;

}
  

void DescriptorWriter::updateSet(VkDevice t_device, VkDescriptorSet t_set){
    for (VkWriteDescriptorSet& write : g_writes) {
        write.dstSet = t_set;
    }

    vkUpdateDescriptorSets(t_device, (uint32_t)g_writes.size(), g_writes.data(), 0, nullptr);
}

void DescriptorWriter::writeBuffer(int t_binding,VkBuffer t_buffer,size_t t_size, size_t t_offset,VkDescriptorType t_type){
    VkDescriptorBufferInfo& info = g_bufferInfos.emplace_back(VkDescriptorBufferInfo{
		.buffer = t_buffer,
		.offset = t_offset,
		.range = t_size
		});

	VkWriteDescriptorSet write {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = t_binding;
	write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
	write.descriptorCount = 1;
	write.descriptorType = t_type;
	write.pBufferInfo = &info;

	g_writes.push_back(write);
}
void DescriptorWriter::writeImage(int t_binding,VkImageView t_image,VkSampler t_sampler , VkImageLayout t_layout,
    VkDescriptorType t_type){
        VkDescriptorImageInfo& info = g_imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = t_sampler,
            .imageView = t_image,
            .imageLayout = t_layout
        });
    
        VkWriteDescriptorSet write {  };
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstBinding = t_binding;
        write.dstSet = VK_NULL_HANDLE; //left empty for now until we need to write it
        write.descriptorCount = 1;
        write.descriptorType = t_type;
        write.pImageInfo = &info;
    
        g_writes.push_back(write);
}
void DescriptorWriter::clear(){
    g_imageInfos.clear();
    g_writes.clear();
    g_bufferInfos.clear();

}