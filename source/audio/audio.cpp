#include "audio.h"

#include <cmath>

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
    decoder = opus_decoder_create(48000, 1, &error);
    opus_check(error);

    const char *device_name = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
    printf("Audio out: %s\n", device_name);
    playback_device = alcOpenDevice(device_name);
    check(playback_device);

    device_name = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    printf("Audio in: %s\n", device_name);
    capture_device = alcCaptureOpenDevice(
        device_name, 48000, AL_FORMAT_MONO16, buffer_count * buffer_size
    );
    check(capture_device);

    alcCaptureStart(capture_device.get());
    check(capture_device);

    context = alcCreateContext(playback_device.get(), nullptr);
    check(playback_device);

    alcMakeContextCurrent(context.get());
    check(playback_device);

    alGenBuffers(std::size(buffers.get()), buffers->data());
    openal_check();

    std::fill(std::begin(buffer_data), std::end(buffer_data), 0.0f);

    ALshort* buffer_data_begin = std::begin(buffer_data);
    for (auto buffer : buffers.get()) {
        alBufferData(
            buffer, AL_FORMAT_MONO16, buffer_data_begin,
            buffer_size * sizeof(ALshort), 48000
        );
        openal_check();
        buffer_data_begin += buffer_size;
    }

    alGenSources(size(sources.get()), sources->data());
    openal_check();

    alSourcef(sources.get()[0], AL_PITCH, 1);
    alSourcef(sources.get()[0], AL_GAIN, 1.0f);
    alSource3f(sources.get()[0], AL_POSITION, 0, 0, 0);
    alSource3f(sources.get()[0], AL_VELOCITY, 0, 0, 0);
    alSourcei(sources.get()[0], AL_LOOPING, AL_FALSE);
    openal_check();

    alSourceQueueBuffers(
        sources.get()[0], std::size(buffers.get()), buffers->data()
    );
    openal_check();

    alSourcePlay(sources.get()[0]);
    openal_check();
}

void audio::update(::client& client) {
    ALshort capture_data[buffer_size];
    std::fill(std::begin(capture_data), std::end(capture_data), 0.0f);
    ALCenum error;

    while (true) {
        alcCaptureSamples(
            capture_device.get(), (ALCvoid *)capture_data, 
            std::size(capture_data)
        );
        error = alcGetError(capture_device.get());
        if (error == ALC_INVALID_VALUE)
            break; // nothing to capture

        openal_check(error);

        client.encoded_audio_in_size = opus_check(opus_encode(
            encoder.get(), capture_data, std::size(capture_data),
            client.encoded_audio_in.data(), client.encoded_audio_in.size()
        ));
        opus_check(opus_decode(
            decoder.get(), client.encoded_audio_out.data(),
            client.encoded_audio_out_size,
            capture_data, std::size(capture_data), 0
        ));

        ALuint unqueued_buffer = 0;
        alSourceUnqueueBuffers(sources.get()[0], 1, &unqueued_buffer);
        error = alGetError();
        if (error == AL_INVALID_VALUE)
            break; // not ready to play new samples, drop them

        openal_check(error);

        alBufferData(
            unqueued_buffer, AL_FORMAT_MONO16, std::begin(capture_data),
            std::size(capture_data) * sizeof(ALshort), 48000
        );
        openal_check();

        alSourceQueueBuffers(sources.get()[0], 1, &unqueued_buffer);
        openal_check();
    }

    ALint state=0;
    alGetSourcei(sources.get()[0], AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(sources.get()[0]);
        openal_check();
    }
}
