#include "panel_startup_logo.h"

extern const uint8_t _binary_panel_startup_logo_a8_start[];

const lv_image_dsc_t panel_startup_logo_image = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_A8,
        .w = 170U,
        .h = 170U,
        .stride = 170U,
    },
    .data_size = 170U * 170U,
    .data = _binary_panel_startup_logo_a8_start,
};
