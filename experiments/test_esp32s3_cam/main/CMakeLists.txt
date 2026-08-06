idf_component_register(SRCS "test_esp32s3_cam.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp32-camera esp_timer esp_driver_gpio nvs_utils esp32_chip_info esp_hw_support)
