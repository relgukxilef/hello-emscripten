#include "audio.h"

#include <opus_defines.h>

audio::audio() {
    int error;
    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);
    opus_check(error);
}
