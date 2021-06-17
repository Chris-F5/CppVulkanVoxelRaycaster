#pragma once

#include <string>

#include <vulkan/vulkan.h>

void handleVkResult(VkResult result, std::string action_description);
