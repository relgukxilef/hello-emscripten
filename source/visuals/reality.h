#pragma once

#include "../utility/xr_resource.h"

enum struct platform {
    opengl, opengl_es, vulkan,
};

struct reality {
    reality(XrInstance instance, platform platform);

    unique_xr_session xr_session;
};
