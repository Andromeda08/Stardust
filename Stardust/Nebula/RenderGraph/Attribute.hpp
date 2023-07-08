#pragma once

#include <fmt/core.h>
#include <Nebula/RenderGraph/Resource.hpp>

namespace Nebula::RenderGraph
{
    class Attribute
    {
    public:
        virtual const std::shared_ptr<Resource>& resource() const = 0;
    };
}