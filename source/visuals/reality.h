#pragma once

#include "../utility/xr_resource.h"

struct reality {
    reality(XrInstance instance, XrSession xr_session);

    XrSession xr_session;
};
