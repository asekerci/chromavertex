/* SPDX-License-Identifier: GPL-3.0-or-later */

/*
 * test_esp32_cam.c - Basic operational tests for the camera
 * 
 * Copyright (C) 2026 Ahmet Sekercioglu 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * We use this program to check the toolchain and to perform
 * some basic tests on the ESP32 chip. It is also used to verify the correct operation of the
 * ESP32 chip and to ensure that the toolchain is functioning properly.
 *
 * Directly flash this program (via a USB cable) to the factory partition.
 */

#include <stdio.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "nvs_utils.h"
#include "esp32_chip_info.h"

static int64_t g_start_time;
static bool g_io02_led_state;
static const bool g_io02_led_active_low = true;

static const char *TAG = "test_esp32s3_cam";

// Pin mapping for common ESP32-S3-CAM boards.
#define CAM_PIN_PWDN    38
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    4
#define CAM_PIN_SIOC    5

#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      10
#define CAM_PIN_D2      8
#define CAM_PIN_D1      9
#define CAM_PIN_D0      11
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    13

static bool g_camera_initialized;

static void log_elapsed_time(int64_t start_time)
{
    int64_t end_time = esp_timer_get_time();
    int64_t elapsed_time = end_time - start_time;

    int64_t elapsed_minutes = elapsed_time / (60 * 1000000);
    elapsed_time %= (60 * 1000000);
    int64_t elapsed_seconds = elapsed_time / 1000000;
    elapsed_time %= 1000000;
    int64_t elapsed_milliseconds = elapsed_time / 1000;
    int64_t elapsed_microseconds = elapsed_time % 1000;

    ESP_LOGI("Elapsed time", "\t%lld min\t%lld sec\t %lld msec\t%lld usec",
             elapsed_minutes, elapsed_seconds, elapsed_milliseconds, elapsed_microseconds);
}

static void initialize_io02_led(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << GPIO_NUM_2,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ESP_ERROR_CHECK(gpio_config(&cfg));

    g_io02_led_state = false;
    gpio_set_level(GPIO_NUM_2, g_io02_led_active_low ? 1 : 0);
}

static void io02_led_toggle(void)
{
    g_io02_led_state = !g_io02_led_state;
    gpio_set_level(GPIO_NUM_2, g_io02_led_active_low ? !g_io02_led_state : g_io02_led_state);
}

static void task_record_time(void *pvParameter)
{
    (void)pvParameter;

    while (1) {
        log_elapsed_time(g_start_time);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static void task_io02_demo(void *arg)
{
    (void)arg;

    while (1) {
        io02_led_toggle();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static esp_err_t initialize_camera(void)
{
    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
        .pin_sccb_sda = CAM_PIN_SIOD,
        .pin_sccb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_D7,
        .pin_d6 = CAM_PIN_D6,
        .pin_d5 = CAM_PIN_D5,
        .pin_d4 = CAM_PIN_D4,
        .pin_d3 = CAM_PIN_D3,
        .pin_d2 = CAM_PIN_D2,
        .pin_d1 = CAM_PIN_D1,
        .pin_d0 = CAM_PIN_D0,
        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,

        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = FRAMESIZE_VGA,
        .jpeg_quality = 12,
        .fb_count = 2,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
        .fb_location = CAMERA_FB_IN_PSRAM,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_camera_init failed: 0x%x", err);
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor != NULL) {
        ESP_LOGI(TAG, "Camera sensor PID: 0x%04x", sensor->id.PID);
    }

    g_camera_initialized = true;
    ESP_LOGI(TAG, "Camera initialized for JPEG VGA capture");
    return ESP_OK;
}

static void task_camera_capture(void *arg)
{
    (void)arg;

    if (!g_camera_initialized) {
        ESP_LOGE(TAG, "Camera is not initialized; exiting capture task");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        int64_t t0 = esp_timer_get_time();
        camera_fb_t *fb = esp_camera_fb_get();
        int64_t t1 = esp_timer_get_time();

        if (fb == NULL) {
            ESP_LOGE(TAG, "Camera capture failed");
        } else {
            ESP_LOGI(TAG,
                     "JPEG captured: %ux%u, %u bytes, %.2f ms",
                     fb->width,
                     fb->height,
                     (unsigned int)fb->len,
                     (double)(t1 - t0) / 1000.0);
            esp_camera_fb_return(fb);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    g_start_time = esp_timer_get_time();

    initialize_io02_led();
    if (ESP_OK != initialize_nvs()) {
        ESP_LOGE(TAG, "Failed to initialize NVS");
        return;
    }
    if (ESP_OK != initialize_camera()) {
        ESP_LOGE(TAG, "Camera initialization failed");
        return;
    }

    get_chip_info(); fflush(stdout);

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    if (pdPASS != xTaskCreate(&task_record_time, "task_record_time", 4096, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create record_time task");
    }

    if (pdPASS != xTaskCreate(&task_io02_demo, "io02_demo", 2048, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create io02_demo task");
    }
 
    if (pdPASS != xTaskCreate(&task_camera_capture, "camera_capture", 4096, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create camera_capture task");
    }
}
