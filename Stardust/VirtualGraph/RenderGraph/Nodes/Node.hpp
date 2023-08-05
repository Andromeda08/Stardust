#pragma once

namespace Nebula::RenderGraph
{
    class Node
    {
    public:
        Node() = default;

        virtual void execute() = 0;

        virtual ~Node() = default;
    };
}