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

inline int opus_check(int error) {
    if (error >= 0) {
        return error;
    } else {
        throw opus_error(error);
    }
}

typedef unique_resource<
    OpusEncoder*, handle_delete<OpusEncoder, opus_encoder_destroy>
> unique_opus_encoder;

typedef unique_resource<
    OpusDecoder*, handle_delete<OpusDecoder, opus_decoder_destroy>
> unique_opus_decoder;
