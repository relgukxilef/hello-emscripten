#include "audio.h"
#include <AL/alc.h>

// emscripten workaround
// https://github.com/emscripten-core/emscripten/issues/9851
#define AL_FORMAT_MONO_FLOAT32                   0x10010
#define AL_FORMAT_STEREO_FLOAT32                 0x10011

#include <opus_defines.h>

audio::audio() {
    int error;

    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);
    opus_check(error);

    const char *device_name = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);

    capture_device = alcCaptureOpenDevice(
        device_name, 48000, AL_FORMAT_MONO_FLOAT32, 1024
    );
    check(capture_device);
}
