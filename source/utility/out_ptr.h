#pragma once

template<class Smart, class Pointer>
struct out_ptr_t {
    out_ptr_t(Smart& smart) : smart(smart), pointer() {}
    ~out_ptr_t() { smart = Smart(pointer); }

    operator Pointer*() { return &pointer; }

    Smart &smart;
    Pointer pointer;
};

template<class Smart>
out_ptr_t<Smart, typename Smart::pointer> out_ptr(Smart& smart) {
    return { smart };
}
