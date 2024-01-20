#pragma once

#include <cstdint>
#include <chrono>

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
    unix_time_point;

struct time_range {
    unix_time_point begin, end;
};

struct salted_hash {
    uint8_t salt[8],  hash[16];
};

struct login_user {
    uint8_t name[64];
};

struct password_login {
    uint32_t user;
    salted_hash password;
    time_range history;
};

struct email_login {
    uint32_t user;
    salted_hash address;
    time_range history;
};
