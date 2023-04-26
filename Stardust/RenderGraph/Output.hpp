#pragma once

namespace sd::rg
{
    class Output
    {
    public:
        virtual void do_thing() {}

        virtual ~Output() = default;
    };
}