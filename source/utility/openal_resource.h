#pragma once

#include <stdexcept>
#include <cstdio>
#include <assert.h>
#include <array>

#include <AL/alc.h>
#include <AL/al.h>

#include "resource.h"

struct openal_error : public std::exception {
    openal_error(ALenum error) noexcept;

    const char *what() const noexcept override;

    ALenum error;
};

struct openalc_error : public std::exception {
    openalc_error(ALCenum error) noexcept;

    const char *what() const noexcept override;

    ALCenum error;
};

inline openal_error::openal_error(ALenum error) noexcept : error(error) {}

inline openalc_error::openalc_error(ALCenum error) noexcept : error(error) {}

template<unsigned N, typename T, auto Deleter>
void openal_array_delete(std::array<T, N>* t) {
    Deleter(N, t->data());
}

typedef unique_resource<
    ALCdevice*, handle_delete<ALCdevice, alcCloseDevice>
> unique_openal_playback_device;

typedef unique_resource<
    ALCdevice*, handle_delete<ALCdevice, alcCaptureCloseDevice>
> unique_openal_capture_device;

typedef unique_resource<
    ALCcontext*, handle_delete<ALCcontext, alcDestroyContext>
> unique_openal_context;

template<std::size_t N>
using unique_openal_buffers = unique_resource<
    std::array<ALuint, N>, openal_array_delete<N, ALuint, alDeleteBuffers>
>;

template<std::size_t N>
using unique_openal_sources = unique_resource<
    std::array<ALuint, N>, openal_array_delete<N, ALuint, alDeleteSources>
>;


inline void openal_check(ALenum error) {
    if (error == AL_NO_ERROR) {
        return;
    } else {
        throw openal_error(error);
    }
}

inline void openalc_check(ALCenum error) {
    if (error == ALC_NO_ERROR) {
        return;
    } else {
        throw openalc_error(error);
    }
}

inline void check(unique_openal_capture_device &device) {
    if (!device) {
        throw std::runtime_error("No capture device");
    }
    openalc_check(alcGetError(device.get()));
}

inline void check(unique_openal_playback_device &device) {
    if (!device) {
        throw std::runtime_error("No playback device");
    }
    openalc_check(alcGetError(device.get()));
}

inline void openal_check() {
    // This function only works after a context has been created
    // If context creation fails there is no way to find out why
    openal_check(alGetError());
}
