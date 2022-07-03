#pragma once

#include <util/nameable.hpp>

namespace krypton::rapi {
    class ISemaphore : public util::Nameable {

    public:
        ~ISemaphore() override = default;

        virtual void create() = 0;
    };
} // namespace krypton::rapi
