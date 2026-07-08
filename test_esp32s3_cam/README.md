# Basic ESP32 Camera Operational Tests

## Procedure

The procedure for generating the firmware binary and flashing is same as
[here](../get_esp32_chip_info/README.md).

## Required Components

Run the following `idf.py` command from the project root to add the
camera handling routines:

```bash
idf.py add-dependency "espressif/esp32-camera^2.1.7"
```

This command writes the dependency information to `idf_component.yml`
and updates `dependencies.lock`.
