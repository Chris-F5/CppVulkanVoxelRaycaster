#pragma once

#include <string>

#include <vulkan/vulkan.h>

VkShaderModule createShaderModule(VkDevice device, std::string shaderSpvFile);
