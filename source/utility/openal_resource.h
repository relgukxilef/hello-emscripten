#pragma once

#include <stdexcept>

#include <AL/alc.h>
#include <assert.h>

#include "resource.h"

struct openal_error : public std::exception {
    openal_error(ALCenum error) noexcept;

    const char *what() const noexcept override;

    ALCenum error;
};

inline openal_error::openal_error(ALCenum error) noexcept : error(error) {}

template<typename T, auto Deleter>
void openal_delete(T** t) {
    assert(Deleter(*t));
}

typedef unique_resource<ALCcontext*, alcDestroyContext> unique_openal_context;

typedef unique_resource<
    ALCdevice*, openal_delete<ALCdevice, alcCloseDevice>
> unique_openal_playback_device;

typedef unique_resource<
    ALCdevice*, openal_delete<ALCdevice, alcCaptureCloseDevice>
> unique_openal_capture_device;

inline void check(unique_openal_capture_device &device) {
    ALCenum error = alcGetError(device.get());
    if (error == ALC_NO_ERROR) {
        return;
    } else {
        throw openal_error(error);
    }
}
