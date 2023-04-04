#pragma once
#include <map>
#include <memory>

namespace sd::rg {
    class RtaoNode : public INode
    {
    public:


    private:
        // An RTAO Node expects 2 inputs:
        // - G-Buffer image
        // - Top level acceleration structure
        std::map<std::string, IInput> m_inputs;

        // An RTAO Node has only one output:
        // - AO buffer image
        std::map<std::string, IOutput> m_outputs;
    };
}