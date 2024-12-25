#include <cinttypes>
#include <cassert>

#include "testing.h"

using namespace std;
using namespace testing;

// Empty definitions to work-around linker error in debug builds with MSVC
extern "C" void __msan_scoped_disable_interceptor_checks(void) {}
extern "C" void __sanitizer_set_death_callback(void) {}
extern "C" void __sanitizer_install_malloc_and_free_hooks(void) {}
extern "C" void __msan_unpoison(void) {}
extern "C" void __msan_unpoison_param(void) {}
extern "C" void __msan_scoped_enable_interceptor_checks(void) {}
extern "C" void __sanitizer_acquire_crash_state(void) {}
extern "C" int LLVMFuzzerCustomCrossOver(void) { return 0; }
extern "C" size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);
extern "C" size_t LLVMFuzzerCustomMutator(
    uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed
) {
    return LLVMFuzzerMutate(Data, Size, MaxSize);
}
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv) { return 0; }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    fuzzer_input = span<const uint8_t>(data, size);

    auto index = any<size_t>();
    if (index < tests().size())
        tests()[index]();
    
    return 0;
}
