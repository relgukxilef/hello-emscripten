#include "audio.h"
#include <AL/alc.h>
#include <AL/alext.h>

#include <opus_defines.h>

audio::audio() {
    int error;

    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);
    opus_check(error);

    const char *devicename = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);

    capture_device = alcCaptureOpenDevice(
        devicename, 48000, AL_FORMAT_MONO_FLOAT32, 1024
    );
    check(capture_device);
}
