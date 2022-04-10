#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <util/large_vector.hpp>

TEST_CASE("Large vector tests", "[large_vector]") {
    // We specifically set the block size to 32 so that we don't have
    // to allocate so much memory for effectively the same test.
    krypton::util::LargeVector<uint32_t, 32> vector;

    SECTION("Basic tests") {
        // Allocate some items. With 64 items, this should create 2 blocks.
        for (size_t i = 0; i < 64; ++i) {
            vector.push_back(i);
        }
        REQUIRE(vector.size() == 64);
        REQUIRE(vector.capacity() == 64); // The next block should only be allocated after the
                                          // others have been fully used.

        REQUIRE(vector[vector.size() - 1] == 63);
    }

    SECTION("Large vector iterator") {
        for (size_t i = 0; i < 64; ++i) {
            vector.push_back(i);
        }

        uint32_t i = 0;
        for (auto item : vector) {
            REQUIRE(item == i);
            ++i;
        }
    }
}
