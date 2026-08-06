# get_esp32_chip_info: Testing ESP32 Boards

## Prerequisites

We assume that you have already installed the [ESP-IDF
toolchain](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/index.html).

Important note: These steps were tested on Linux. The procedure will
differ on Windows (Perhaps a Linux subsystem installation could be useful (WSL) here?
Please check [this webpage](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/wsl.html).

## After Installation

If we assume you have installed the esp-idf toolchain v5.5.1 under the
`$HOME/lib` directory, create an alias like this one:

```bash
alias init_idf='source $HOME/lib/esp/v5.5.1/esp-idf/export.sh'
```
(Adjust your alias definition according to your installation directory and ESP-IDF 
version)

## Testing an ESP32-S3-CAM Board

We use ESP32-S3-CAM boards for our VertexBot designs. See the [ChromaVertex
Project](https://sekerci.info/~ahmet/networked_robotics/deep_dive.html)
for more information.

Open a terminal window, run the alias you have just defined `init_idf`,
`cd` to the top-level project directory, that is, the directory containing
this README.md file. Then run the following commands.

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
