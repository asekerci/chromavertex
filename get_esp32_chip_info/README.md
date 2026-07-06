# get_esp32_chip_info: Testing ESP32 Boards

## Prerequisites

We assume that you have already installed the [ESP-IDF
toolchain](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).

## Testing an ESP32-S3-CAM Board

We use ESP32-S3-CAM boards for our VertexBot designs. See the [ChromaVertex
Project](https://sekerci.info/~ahmet/networked_robotics/deep_dive.html)
for more information.

Run the following commands from the top-level project directory, that is,
the directory containing this README.md file.

These steps were tested on Linux. The procedure will differ on Windows.

### Generate `sdkconfig`

The initial configuration generates the full `sdkconfig` automatically
from the settings in `sdkconfig.defaults`:

```bash
idf.py set-target esp32s3
idf.py reconfigure
```

You only need to do this once, as long as `sdkconfig.defaults` does
not change.

### Generate the Binary and Download It to the Board

GPIO48 on the board is connected to an RGB LED, which we control using
Espressif’s `led_strip` component. Add the dependency before building:

```bash
idf.py add-dependency "espressif/led_strip^3.0.3"
idf.py build
```

To flash the binary, make sure the board is connected to the computer
via a USB cable:

```bash
idf.py flash monitor
```
