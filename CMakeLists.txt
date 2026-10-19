cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_homey_wall_panel)

# V32 diagnostic-only local RGB source replacement.
include(cmake/panel_rgb_diag_override.cmake)
# Patch102: hash-pinned, build-local Waveshare BSP PCLK override.
include(cmake/panel_bsp_pclk12_override.cmake)
