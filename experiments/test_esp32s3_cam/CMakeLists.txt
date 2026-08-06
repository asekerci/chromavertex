# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Set the path to your extra components
set(EXTRA_COMPONENT_DIRS 
		../components/esp32_chip_info 
        ../components/nvs_utils
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test_esp32s3_cam)
