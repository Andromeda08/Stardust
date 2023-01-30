#include "ContextBuilder.hpp"

namespace sdvk
{

    ContextBuilder& ContextBuilder::add_instance_extensions(const std::vector<const char*>& extensions)
    {
        for (const auto& e : extensions)
        {
            _options.instance_extensions.insert(e);
        }
        return *this;
    }

    ContextBuilder& ContextBuilder::add_instance_extensions(const std::initializer_list<const char*>& extensions)
    {
        for (const auto& e : extensions)
        {
            _options.instance_extensions.insert(e);
        }
        return *this;
    }

    ContextBuilder& ContextBuilder::set_validation(bool validation)
    {
        _options.validation = validation;
        return *this;
    }

    ContextBuilder&ContextBuilder::set_debug_utils(bool debug)
    {
        _options.debug = debug;
        return *this;
    }

    ContextBuilder& ContextBuilder::with_surface(GLFWwindow* target)
    {
        _options.with_surface = true;
        _options.window = target;
        return *this;
    }

    ContextBuilder& ContextBuilder::add_device_extensions(const std::initializer_list<const char*>& extensions)
    {
        for (const auto& e : extensions)
        {
            _options.device_extensions.insert(e);
        }
        return *this;
    }

    ContextBuilder &ContextBuilder::add_raytracing_extensions(bool flag)
    {
        if (flag)
        {
            add_device_extensions({
                VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
                VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,

                VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME
            });

            _options.raytracing = true;
        }
        return *this;
    }

    std::unique_ptr<Context> ContextBuilder::create_context() const
    {
        return std::make_unique<Context>(_options);
    }
}