cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_homey_wall_panel)

# V32 diagnostic-only local RGB source replacement.
include(cmake/panel_rgb_diag_override.cmake)
