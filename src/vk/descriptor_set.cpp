#include "descriptor_set.hpp"

#include <iostream>

#include "exceptions.hpp"

bool descriptorTypeNeedsImageInfo(VkDescriptorType type)
{
    return type == VK_DESCRIPTOR_TYPE_SAMPLER ||
           type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
           type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT ||
           type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
}

bool descriptorTypeNeedsBufferInfo(VkDescriptorType type)
{
    return type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
           type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
}

VkDescriptorSetLayoutBinding createLayoutBinding(const DescriptorCreateInfo *bindingInfo)
{
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = bindingInfo->binding;
    layoutBinding.descriptorType = bindingInfo->type;
    // Size of descriptor array of this binding
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = bindingInfo->stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;

    return layoutBinding;
}

VkDescriptorPoolSize createPoolSize(const DescriptorCreateInfo *bindingInfo, uint32_t setCount)
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = bindingInfo->type;
    // If type is VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT then descriptorCount
    // is the number of bytes to allocate for descriptors of this type.
    poolSize.descriptorCount = setCount;

    return poolSize;
}

VkWriteDescriptorSet createDescriptorSetWrite(
    const DescriptorCreateInfo *bindingInfo,
    VkDescriptorSet set,
    size_t index,
    VkDescriptorImageInfo *imageInfo,
    VkDescriptorBufferInfo *bufferInfo)
{
    VkWriteDescriptorSet descriptorSetWrite{};
    descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWrite.pNext = nullptr;
    descriptorSetWrite.dstSet = set;
    descriptorSetWrite.dstBinding = bindingInfo->binding;
    // If the descriptor binding identified by dstSet and dstBinding has a descriptor
    // type of VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT then dstArrayElement
    // specifies the starting byte offset within the binding.
    descriptorSetWrite.dstArrayElement = 0;
    // If the descriptor binding identified by dstSet and dstBinding has a descriptor
    // type of VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, then descriptorCount
    // specifies the number of bytes to update.
    descriptorSetWrite.descriptorCount = 1;
    descriptorSetWrite.descriptorType = bindingInfo->type;

    if (descriptorTypeNeedsImageInfo(bindingInfo->type))
    {
        imageInfo->sampler = nullptr;
        imageInfo->imageView = bindingInfo->imageViews[index];
        imageInfo->imageLayout = bindingInfo->imageLayout;

        descriptorSetWrite.pImageInfo = imageInfo;
        descriptorSetWrite.pBufferInfo = nullptr;
        descriptorSetWrite.pTexelBufferView = nullptr;
    }
    else if (descriptorTypeNeedsBufferInfo(bindingInfo->type))
    {
        bufferInfo->buffer = bindingInfo->buffers[index];
        bufferInfo->offset = 0;
        bufferInfo->range = VK_WHOLE_SIZE;

        descriptorSetWrite.pImageInfo = nullptr;
        descriptorSetWrite.pBufferInfo = bufferInfo;
        descriptorSetWrite.pTexelBufferView = nullptr;
    }
    else
    {
        throw std::runtime_error("unsupported descriptor type when creating descriptor set write");
    }

    return descriptorSetWrite;
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, std::vector<DescriptorCreateInfo> descriptorInfos)
{
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings(descriptorInfos.size());
    for (int i = 0; i < descriptorInfos.size(); i++)
        layoutBindings[i] = createLayoutBinding(&descriptorInfos[i]);

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.pNext = nullptr;
    descriptorSetLayoutInfo.flags = 0;
    descriptorSetLayoutInfo.bindingCount = layoutBindings.size();
    descriptorSetLayoutInfo.pBindings = layoutBindings.data();

    VkDescriptorSetLayout descriptorSetLayout;
    handleVkResult(
        vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &descriptorSetLayout),
        "creating descriptor set layout");

    return descriptorSetLayout;
}

VkDescriptorPool createDescriptorPool(VkDevice device, std::vector<DescriptorCreateInfo> descriptorInfos, uint32_t setCount)
{
    std::vector<VkDescriptorPoolSize> poolSizes(descriptorInfos.size());

    for (int i = 0; i < descriptorInfos.size(); i++)
        poolSizes[i] = createPoolSize(&descriptorInfos[i], setCount);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = 0;
    poolInfo.maxSets = setCount;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();

    VkDescriptorPool descriptorPool;
    handleVkResult(
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool),
        "creating descriptor pool");

    return descriptorPool;
}

std::vector<VkDescriptorSet> allocateDescriptorSets(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout,
    uint32_t setCount)
{

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts(setCount, descriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetInfo{};
    descriptorSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetInfo.pNext = nullptr;
    descriptorSetInfo.descriptorPool = descriptorPool;
    descriptorSetInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    descriptorSetInfo.pSetLayouts = descriptorSetLayouts.data();

    std::vector<VkDescriptorSet> descriptorSets(setCount);
    handleVkResult(
        vkAllocateDescriptorSets(device, &descriptorSetInfo, descriptorSets.data()),
        "allocating descriptor sets");
    return descriptorSets;
}

void writeDescriptorSets(VkDevice device, std::vector<VkDescriptorSet> descriptorSets, std::vector<DescriptorCreateInfo> descriptorInfos)
{
    for (int i = 0; i < descriptorSets.size(); i++)
    {
        std::vector<VkDescriptorImageInfo> imageInfos(descriptorInfos.size());
        std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorInfos.size());
        std::vector<VkWriteDescriptorSet> descriptorWrites(descriptorInfos.size());
        for (int j = 0; j < descriptorInfos.size(); j++)
            descriptorWrites[j] = createDescriptorSetWrite(&descriptorInfos[j], descriptorSets[i], i, &imageInfos[j], &bufferInfos[j]);
        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

DescriptorSets createDescriptorSets(VkDevice device, std::vector<DescriptorCreateInfo> descriptorInfos, uint32_t setCount)
{

    DescriptorSets descriptorSets{};

    descriptorSets.layout = createDescriptorSetLayout(device, descriptorInfos);
    descriptorSets.pool = createDescriptorPool(device, descriptorInfos, setCount);
    descriptorSets.sets =
        allocateDescriptorSets(device, descriptorSets.pool, descriptorSets.layout, setCount);
    writeDescriptorSets(device, descriptorSets.sets, descriptorInfos);
    return descriptorSets;
}