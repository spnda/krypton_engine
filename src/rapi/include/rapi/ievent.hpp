#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class IEvent : public util::Nameable {
    public:
        ~IEvent() override = default;

        virtual void create(uint64_t initialValue) = 0;
    };
} // namespace krypton::rapi
