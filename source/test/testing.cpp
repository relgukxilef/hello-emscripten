#include "testing.h"

namespace testing {

    span<const uint8_t> fuzzer_input;

    vector<function<void()>> &tests() {
        static vector<function<void()>> global_tests;
        return global_tests;
    }

    test::test(function<void()> run) {
        tests().push_back(move(run));
    }

    uint8_t any_byte() {
        if (fuzzer_input.empty())
            return 0;
        auto result = fuzzer_input[0];
        fuzzer_input = fuzzer_input.subspan(1);
        return result;
    }
}