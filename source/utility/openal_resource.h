#pragma once

#include <stdexcept>
#include <cstdio>
#include <assert.h>
#include <array>

#include <AL/alc.h>
#include <AL/al.h>

#include "resource.h"

struct openal_error : public std::exception {
    openal_error(ALCenum error) noexcept;

    const char *what() const noexcept override;

    ALCenum error;
};

inline openal_error::openal_error(ALCenum error) noexcept : error(error) {
    std::puts(what());
    std::puts("\n");
}

template<typename T, auto Deleter>
void openal_delete(T** t) {
    Deleter(*t);
}

template<unsigned N, typename T, auto Deleter>
void openal_array_delete(std::array<T, N>* t) {
    Deleter(N, begin(*t));
}

typedef unique_resource<
    ALCdevice*, openal_delete<ALCdevice, alcCloseDevice>
> unique_openal_playback_device;

typedef unique_resource<
    ALCdevice*, openal_delete<ALCdevice, alcCaptureCloseDevice>
> unique_openal_capture_device;

typedef unique_resource<
    ALCcontext*, openal_delete<ALCcontext, alcDestroyContext>
> unique_openal_context;

template<std::size_t N>
using unique_openal_buffers = unique_resource<
    std::array<ALuint, N>, openal_array_delete<N, ALuint, alDeleteBuffers>
>;

template<std::size_t N>
using unique_openal_sources = unique_resource<
    std::array<ALuint, N>, openal_array_delete<N, ALuint, alDeleteSources>
>;


// TODO: ALCenum and ALenum error codes have different meaning but the same type
inline void openal_check(ALCenum error) {
    if (error == ALC_NO_ERROR) {
        return;
    } else {
        throw openal_error(error);
    }
}

inline void check(unique_openal_capture_device &device) {
    openal_check(alcGetError(device.get()));
}

inline void check(unique_openal_playback_device &device) {
    openal_check(alcGetError(device.get()));
}

inline void openal_check() {
    openal_check(alGetError());
}
