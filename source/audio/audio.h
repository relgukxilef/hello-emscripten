#pragma once

#include <ranges>

#include <opus.h>
#include <AL/al.h>

#include "../state/client.h"
#include "../utility/opus_resource.h"
#include "../utility/openal_resource.h"

constexpr std::size_t buffer_count = 4;
constexpr std::size_t buffer_size = 2880; // largest Opus frame size
constexpr std::size_t sources_count = 32;

struct audio {
    audio();

    void update(::client &client);

    unique_opus_encoder encoder;
    unique_opus_decoder decoders[sources_count];

    unique_openal_capture_device capture_device;
    unique_openal_playback_device playback_device;
    unique_openal_context context;

    ALshort buffer_data[buffer_count * buffer_size * sources_count];

    unique_openal_buffers<buffer_count * sources_count> buffers;
    unique_openal_sources<sources_count> sources;

    bool play_sine = false;
};
