#pragma once

#include <vector>
#include <functional>
#include <utility>
#include <cinttypes>
#include <span>
#include <concepts>

namespace testing {
    using namespace std;

    extern span<const uint8_t> fuzzer_input;

    vector<function<void()>> &tests();

    struct test {
        test(function<void()> run);
    };

    uint8_t any_byte();

    template<integral T>
    T any() {
        T result = 0;
        for (int i = 0; i < sizeof(T); i++)
            result = (result << 8) + any_byte();
        return result;
    }
}
