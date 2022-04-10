#include <catch2/catch_test_macros.hpp>

#include <util/free_list.hpp>

TEST_CASE("Free list tests", "[free_list]") {
    // Every section has a new list created for it.
    krypton::util::FreeList<int32_t, "integer"> list;

    REQUIRE(list.data() != nullptr);
    REQUIRE(list.size() == 1); // There's always a "HEAD" in a FreeList.

    SECTION("Check if a handle can be created") {
        auto handle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        REQUIRE(list.size() == 2);
        REQUIRE(handle.getIndex() == 1);
        REQUIRE(handle.getGeneration() == 0);
    }

    SECTION("Check if the handle is properly detected as invalid.") {
        auto handle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        list.removeHandle(handle);
        REQUIRE_THROWS_AS(list.getFromHandle(handle), std::invalid_argument);
    }

    // We create a first handle, then remove it to check if the list properly reuses the slot for
    // another handle we create. It also checks if the generation value is properly incremented.
    SECTION("Check if the hole is properly reused") {
        auto oldHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        REQUIRE(list.size() == 2);
        REQUIRE(oldHandle.getIndex() == 1);
        REQUIRE(oldHandle.getGeneration() == 0);

        list.removeHandle(oldHandle);
        auto newHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        REQUIRE(list.size() == 2);
        REQUIRE(newHandle.getGeneration() == 1);
        REQUIRE_THROWS_AS(list.getFromHandle(oldHandle), std::invalid_argument);
    }

    // Here we create two handles, at index 1 and 2. Then we destroy the first one and create a new
    // handle to check if the new handle goes into the previously destroyed slot and does not
    // create a new slot in the list.
    SECTION("Check if holes work properly") {
        auto firstHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        auto secondHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());

        list.removeHandle(firstHandle);
        REQUIRE_THROWS_AS(list.getFromHandle(firstHandle), std::invalid_argument);

        auto newHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        REQUIRE(newHandle.getIndex() == firstHandle.getIndex());
    }

    SECTION("Check that after removing, the value is properly removed") {
        auto handle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        list.getFromHandle(handle) = 5; // some arbitrary value
        list.removeHandle(handle);

        auto newHandle = list.getNewHandle(std::make_shared<krypton::util::ReferenceCounter>());
        REQUIRE(list.getFromHandle(newHandle) == 0);
    }

    // Various tests for if the Handle and ReferenceCounter work properly with move, copy and other
    // semantics.
    SECTION("Check if the ReferenceCounter works") {
        auto refCounter = std::make_shared<krypton::util::ReferenceCounter>();
        auto handle = list.getNewHandle(refCounter);
        REQUIRE(refCounter->count() == 1);

        // Here we create a scope-local copy of our handle, which temporarily increments the
        // reference counter.
        {
            auto copy1(handle); // NOLINT
            REQUIRE(refCounter->count() == 2);
        }
        REQUIRE(refCounter->count() == 1);

        auto copy2 = std::move(handle);
        REQUIRE(refCounter->count() == 1);

        handle = copy2;
        REQUIRE(refCounter->count() == 2);
    }
}
