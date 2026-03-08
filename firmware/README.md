# firmware/

ESP32-S3 firmware for the DIY Graphing Calculator.

## Structure (planned)

- **micropython/** — MicroPython application code (UI, input handling, app logic)
- **c_modules/** — C extension modules (Eigenmath CAS, tinyexpr, display drivers)
- **config/** — Build configuration, partition tables, sdkconfig

## Toolchain

- ESP-IDF + MicroPython (for C module compilation)
- MicroPython with frozen modules for the application layer
