#include <iostream>

#include "window/window.h"
#include "utility/vulkan_resource.h"

int main() {
    window w;

    unique_surface s(w.get_surface());

    std::cout << "Servus Welt!" << std::endl;
    return 0;
}