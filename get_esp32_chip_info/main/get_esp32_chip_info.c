/* SPDX-License-Identifier: GPL-3.0-or-later */

/*
 * get_esp32_chip_info.c - ESP32 chip information utility for SynchroSpark
 * 
 * Copyright (C) 2025-2026 Ahmet Sekercioglu and Ismet Atalar
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
#include "esp_timer.h"
#include "led_strip.h"
#include "nvs_utils.h"
#include "esp32_chip_info.h"

static int64_t g_start_time;
static bool g_io02_led_state;
static const bool g_io02_led_active_low = true;
static led_strip_handle_t g_rgb_strip = NULL;

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

static void initialize_rgb_led(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        },
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 64,
        .flags = {
            .with_dma = false,
        },
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &g_rgb_strip));
    ESP_ERROR_CHECK(led_strip_clear(g_rgb_strip));
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

static void task_rgb_demo(void *arg)
{
    (void)arg;

    while (1) {
        led_strip_set_pixel(g_rgb_strip, 0, 32, 0, 0);
        led_strip_refresh(g_rgb_strip);
        vTaskDelay(pdMS_TO_TICKS(400));

        led_strip_set_pixel(g_rgb_strip, 0, 0, 32, 0);
        led_strip_refresh(g_rgb_strip);
        vTaskDelay(pdMS_TO_TICKS(400));

        led_strip_set_pixel(g_rgb_strip, 0, 0, 0, 32);
        led_strip_refresh(g_rgb_strip);
        vTaskDelay(pdMS_TO_TICKS(400));

        led_strip_clear(g_rgb_strip);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

void app_main(void)
{
    char *TAG = "app_main";

    g_start_time = esp_timer_get_time();

    initialize_io02_led();
    initialize_rgb_led();

    if (ESP_OK != initialize_nvs()) {
        ESP_LOGE(TAG, "Failed to initialize NVS");
        return;
    }

    get_chip_info();
    fflush(stdout);

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    if (pdPASS != xTaskCreate(&task_record_time, "task_record_time", 4096, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create record_time task");
    }

    if (pdPASS != xTaskCreate(&task_io02_demo, "io02_demo", 2048, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create io02_demo task");
    }

    if (pdPASS != xTaskCreate(&task_rgb_demo, "rgb_demo", 4096, NULL, 5, NULL)) {
        ESP_LOGE(TAG, "Failed to create rgb_demo task");
    }
}