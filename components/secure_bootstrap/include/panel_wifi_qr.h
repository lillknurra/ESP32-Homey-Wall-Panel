#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lvgl.h"
#include "secure_bootstrap.h"

#define PANEL_WIFI_QR_STARTUP_SIZE 180
#define PANEL_WIFI_QR_DIALOG_SIZE 180
#define PANEL_WIFI_QR_MAX_PAYLOAD_SIZE 209U

bool panel_wifi_qr_build_payload(
    const char *ssid, const char *password,
    char *output, size_t output_capacity, size_t *output_length);
bool panel_wifi_qr_encode(lv_obj_t *qr, const char *ssid, const char *password);
bool panel_wifi_qr_set_size(lv_obj_t *qr, int32_t size);
int32_t panel_wifi_qr_frame_padding(int32_t qr_size);
bool panel_wifi_qr_layout_frame(lv_obj_t *frame, lv_obj_t *qr, int32_t qr_size);
bool panel_wifi_qr_layout_startup(
    lv_obj_t *screen, lv_obj_t *startup_logo, lv_obj_t *title, lv_obj_t *instruction,
    lv_obj_t *code_caption, lv_obj_t *code, lv_obj_t *frame, lv_obj_t *qr);
bool panel_wifi_qr_startup_status_owned(
    secure_bootstrap_session_phase_t phase, lv_obj_t *screen, lv_obj_t *frame);
bool panel_wifi_qr_clear_stale_startup(
    secure_bootstrap_session_phase_t phase, lv_obj_t *screen, lv_obj_t *frame,
    lv_obj_t *qr, lv_obj_t *code_caption, lv_obj_t *code);
void panel_wifi_qr_clear_startup(
    lv_obj_t *frame, lv_obj_t *qr, lv_obj_t *code_caption, lv_obj_t *code);
