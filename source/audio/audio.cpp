#include "audio.h"
#include "../utility/trace.h"

#include <cmath>
#include <ctime>

#include <AL/alc.h>

// emscripten workaround
// https://github.com/emscripten-core/emscripten/issues/9851
#define AL_FORMAT_MONO_FLOAT32                   0x10010
#define AL_FORMAT_STEREO_FLOAT32                 0x10011

#include <opus_defines.h>

audio::audio() {
    scope_trace trace;
    int error;
    encoder = opus_encoder_create(48000, 1, OPUS_APPLICATION_VOIP, &error);
    opus_check(error);
    for (auto &decoder : decoders) {
        decoder = opus_decoder_create(48000, 1, &error);
        opus_check(error);
    }

    playback_device = alcOpenDevice(NULL);
    check(playback_device);

    const char *device_name = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    printf("Audio in: %s\n", device_name);
    capture_device = alcCaptureOpenDevice(
        device_name, 48000, AL_FORMAT_MONO16, buffer_count * buffer_size
    );
    error = alcGetError(capture_device.get());
    if (error == ALC_OUT_OF_MEMORY) {
        // This error also indicates that the user hasn't interacted with the
        // page yet. 
        // TODO: create device on unmute
        capture_device.reset();
    } else {
        openalc_check(error);

        alcCaptureStart(capture_device.get());
        check(capture_device);
    }

    context = alcCreateContext(playback_device.get(), nullptr);
    check(playback_device);

    alcMakeContextCurrent(context.get());
    check(playback_device);

    alGenBuffers(ALsizei(std::size(buffers.get())), buffers->data());
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

    auto *buffer = buffers.get().data();
    for (auto source : sources.get()) {
        alSourcef(source, AL_PITCH, 1);
        alSourcef(source, AL_GAIN, 1.0f);
        alSource3f(source, AL_POSITION, 0, 0, 0);
        alSource3f(source, AL_VELOCITY, 0, 0, 0);
        alSourcei(source, AL_LOOPING, AL_FALSE);
        alSourceQueueBuffers(source, buffer_count, buffer);
        alSourcePlay(source);
        buffer += buffer_count;
    }

    openal_check();
}

void audio::update(::client& client) {
    scope_trace trace;
    ALshort capture_data[buffer_size];
    ALCenum error = ALC_NO_ERROR;

    bool audio_available = false;

    static unsigned sample_count = 0;
    static unsigned frequency = 220 + std::time(nullptr) % 220;

    while (client.encoded_audio_in_size == 0) {
        if (!capture_device)
            break;

        ALint samples = 0;
        alcGetIntegerv(capture_device.get(), ALC_CAPTURE_SAMPLES, 1, &samples);
        if (samples < std::size(capture_data))
            break;
        
        alcCaptureSamples(
            capture_device.get(), (ALCvoid *)capture_data, 
            std::size(capture_data)
        );
        error = alcGetError(capture_device.get());

        if (error == ALC_INVALID_DEVICE)
            // On web this means user has not given permission
            break;
            
        openalc_check(error);

        audio_available = true;

        // For now only read one buffer per frame
        break;
    }

    if (audio_available) {
        if (play_sine) {
            sample_count += std::size(capture_data);
            for (unsigned i = 0; i < std::size(capture_data); ++i) {
                // write sine with random frequency into buffer for testing
                float sine = 
                    sinf((sample_count + i) * 2 * frequency * 3.1415f / 48000);
                capture_data[i] = static_cast<int16_t>(sine * 0.1f * 32768);
            }
        }

        scope_trace trace;

        // TODO: should it be allowed to send multiple packets?
        // packets are not delimited, packet sizes would need to be included
        client.encoded_audio_in_size = opus_check(opus_encode(
            encoder.get(), capture_data, std::size(capture_data),
            client.encoded_audio_in.data(), client.encoded_audio_in.size()
        ));
    }

    for (int i = 0; i < client.users.position.size(); i++) {
        scope_trace trace;

        if (i >= sources_count)
            break;

        if (client.users.encoded_audio_out_size[i] == 0)
            continue;

        // TODO: number of samples in packet could differ from buffer_size
        opus_check(opus_decode(
            decoders[i].get(), client.users.encoded_audio_out[i].data(), 
            client.users.encoded_audio_out_size[i],
            capture_data, std::size(capture_data), 0
        ));
        client.users.encoded_audio_out_size[i] = 0;
        ALint processed = 0;
        alGetSourcei(sources.get()[i], AL_BUFFERS_PROCESSED, &processed);
        openal_check();
        // drop samples if not ready to play them
        if (processed > 0) {
            ALuint unqueued_buffer = 0;
            alSourceUnqueueBuffers(sources.get()[i], 1, &unqueued_buffer);
            openal_check();

            alBufferData(
                unqueued_buffer, AL_FORMAT_MONO16, std::begin(capture_data),
                std::size(capture_data) * sizeof(ALshort), 48000
            );
            openal_check();

            alSourceQueueBuffers(sources.get()[i], 1, &unqueued_buffer);
            openal_check();
        }
        
        ALint state = 0;
        alGetSourcei(sources.get()[i], AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            alSourcePlay(sources.get()[i]);
            openal_check();
        }
    }
}
