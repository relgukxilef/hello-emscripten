#pragma once

#include <stdexcept>

#include <opus.h>

#include "resource.h"

struct opus_error : public std::exception {
    opus_error(int error) noexcept;

    const char *what() const noexcept override;

    int error;
};

inline opus_error::opus_error(int error) noexcept : error(error) {}

inline void opus_check(int error) {
    if (error == OPUS_OK) {
        return;
    } else {
        throw opus_error(error);
    }
}

inline void opus_encoder_delete(OpusEncoder** encoder) {
    opus_encoder_destroy(*encoder);
}

typedef unique_resource<OpusEncoder*, opus_encoder_delete> unique_opus_encoder;
