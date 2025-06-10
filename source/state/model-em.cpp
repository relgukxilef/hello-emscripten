#include "model.h"

#include <emscripten.h>

EM_ASYNC_JS(
    const char*, read_png_js, 
    (
        const unsigned char* begin, const unsigned char* end, 
        unsigned* width, unsigned* height
    ), 
    {
        const image = new Image();

        promise = new Promise((resolve, reject) => {
            image.onload = () => {
                resolve();
            };
            image.onerror = (e) => {
                console.error('Failed to load image: ' + e.message);
                resolve();
            };
        });

        image.src = URL.createObjectURL(
            new Blob([Module.HEAPU8.slice(begin, end)], {type: 'image/png'})
        );

        await promise;

        const canvas = document.createElement('canvas');
        const context = canvas.getContext('2d');

        canvas.width = image.width;
        canvas.height = image.height;

        context.drawImage(image, 0, 0);

        const pixels = context.getImageData(
            0, 0, canvas.width, canvas.height, {colorSpace: "srgb"}
        ).data;

        const pixels_pointer = Module._malloc(pixels.length);
        Module.HEAPU8.set(pixels, pixels_pointer);
        Module.HEAPU32.set([image.width], width >> 2);
        Module.HEAPU32.set([image.height], height >> 2);

        return pixels_pointer;
    }
);

std::vector<uint8_t> read_png(
    std::ranges::subrange<uint8_t*> file,
    unsigned &width, unsigned &height
) {
    const char* pixels = read_png_js(file.begin(), file.end(), &width, &height);
    std::vector<uint8_t> vector(pixels, pixels + width * height * 4);
    free((void*)pixels);
    return vector;
}
