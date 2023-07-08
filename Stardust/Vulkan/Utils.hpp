#pragma once

#include <vulkan/vulkan.hpp>

namespace sdvk::util
{
    static void name_vk_object(std::string const& name, uint64_t object_handle, vk::ObjectType object_type,
                               vk::Device const& device)
    {
        #ifdef SD_DEBUG
        vk::DebugUtilsObjectNameInfoEXT name_info;
        name_info.setPObjectName(name.c_str());
        name_info.setObjectHandle(object_handle);
        name_info.setObjectType(object_type);
        auto r = device.setDebugUtilsObjectNameEXT(&name_info);
        #endif
    }
}

namespace Nebula::Vulkan
{
}