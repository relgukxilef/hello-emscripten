#include "openal_resource.h"
#include <AL/al.h>

const char* openal_error::what() const noexcept {
    switch (error) {
    case AL_INVALID_NAME:
        return "Invalid name (ID) passed to an AL call.";
    case AL_INVALID_ENUM:
        return "Invalid enumeration passed to AL call.";
    case AL_INVALID_VALUE:
        return "Invalid value passed to AL call.";
    case AL_INVALID_OPERATION:
        return "Illegal AL call.";
    case AL_OUT_OF_MEMORY:
        return "Not enough memory to execute the AL call.";
    default:
        return "Other openal error.";
    }
}
