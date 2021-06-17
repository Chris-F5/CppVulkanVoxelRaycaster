#include "shader_module.hpp"

#include <vector>
#include <fstream>

#include "exceptions.hpp"

std::vector<char> readFile(const std::string filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

VkShaderModule createShaderModuleFromSpv(VkDevice device, const std::vector<char> spv)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(spv.data());

    VkShaderModule shaderModule;
    handleVkResult(
        vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule),
        "creating shader module");
    return shaderModule;
}

VkShaderModule createShaderModule(VkDevice device, std::string shaderSpvFile)
{
    auto spv = readFile(shaderSpvFile);
    return createShaderModuleFromSpv(device, spv);
}
