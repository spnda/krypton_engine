#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <util/large_vector.hpp>

TEST_CASE("Large vector tests", "[large_vector]") {
    constexpr auto blockSize = 32;

    // We specifically set the block size to 32 so that we don't have
    // to allocate so much memory for effectively the same test.
    krypton::util::LargeVector<uint32_t, blockSize> vector;

    SECTION("Basic tests") {
        // Allocate some items. With 64 items, this should create 2 blocks.
        for (size_t i = 0; i < blockSize * 2; ++i) {
            vector.push_back(i);
        }
        REQUIRE(vector.size() == blockSize * 2);
        REQUIRE(vector.capacity() == blockSize * 2); // The next block should only be allocated after the
                                                     // others have been fully used.

        REQUIRE(vector[vector.size() - 1] == blockSize * 2 - 1);
    }

    SECTION("Large vector iterator") {
        for (size_t i = 0; i < blockSize * 2; ++i) {
            vector.push_back(i);
        }

        uint32_t i = 0;
        for (auto item : vector) {
            REQUIRE(item == i);
            ++i;
        }
    }
}
