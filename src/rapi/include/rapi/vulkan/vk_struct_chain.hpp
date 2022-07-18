#pragma once

#include <tuple>

struct VkBaseOutStructure;

namespace krypton::rapi::vk {
    // clang-format off
    template<typename T>
    concept hasPNext = requires (T t) {
        t.pNext;
        offsetof(T, pNext) == offsetof(VkBaseOutStructure, pNext);
    };
    // clang-format on

    template <hasPNext... T>
    class StructureChain final {
        std::tuple<T...> structs;

    public:
        explicit StructureChain(T const&... structs) : structs(static_cast<const T&&>(structs)...) {}

        [[nodiscard]] auto getPointer() const noexcept -> std::enable_if_t<sizeof...(T) != 0, const void*> {
#ifdef ZoneScoped
            ZoneScoped;
#endif
            return std::get<0>(structs).pNext;
        }

        auto link() -> std::enable_if_t<sizeof...(T) != 0, void> {
#ifdef ZoneScoped
            ZoneScoped;
#endif
            std::apply(
                [](auto&&... args) {
                    std::array<VkBaseOutStructure*, sizeof...(args)> list = { reinterpret_cast<VkBaseOutStructure*>(&args)... };
                    for (auto i = 0ULL; i < sizeof...(args) - 1; ++i) {
                        list[i]->pNext = list[i + 1];
                    }
                },
                structs);
        }
    };
} // namespace krypton::rapi::vk
