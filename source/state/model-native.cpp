#include "model.h"

#include <png.h>

std::vector<uint8_t> read_png(
    std::ranges::subrange<uint8_t*> file,
    unsigned &width, unsigned &height
) {
    // TODO: create RAII wrapper
    png_structp png = png_create_read_struct(
        PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr
    );
    parse_check(png);
    png_infop info = png_create_info_struct(png);
    parse_check(info);

    if (!setjmp(png_jmpbuf(png))) {
        png_set_read_fn(
            png, &file, [](png_structp png, png_bytep destination, size_t size){
                auto view = 
                    (std::ranges::subrange<uint8_t*> *)png_get_io_ptr(png);
                std::copy(view->data(), view->data() + size, destination);
                *view = {view->begin() + size, view->end()};
            }
        );

        png_read_info(png, info);

        width = png_get_image_width(png, info);
        height = png_get_image_height(png, info);
        auto color_type = png_get_color_type(png, info);
        auto bit_depth  = png_get_bit_depth(png, info);

        if (bit_depth == 16)
            png_set_strip_16(png);

        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png);

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png);

        if (png_get_valid(png, info, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png);

        if (
            color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE
        )
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

        if (
            color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA
        )
            png_set_gray_to_rgb(png);

        png_read_update_info(png, info);

        std::vector<uint8_t> pixels(width * height * 4);
        std::vector<uint8_t*> rows(height);
        for (auto r = 0u; r < height; r++) {
            rows[r] = pixels.data() + r * width * 4;
        }

        png_read_image(png, rows.data());
        
        return pixels;
    }

    png_destroy_read_struct(&png, &info, nullptr);
    return {};
}
