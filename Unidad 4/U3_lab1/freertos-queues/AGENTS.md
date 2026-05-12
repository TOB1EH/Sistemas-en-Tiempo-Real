# AGENTS.md — freertos-queues

ESP32 FreeRTOS project studying queue-based inter-task communication.

## Directivas para agentes

- **Respuestas en español**: todas las respuestas, explicaciones y descripciones deben estar en español. El razonamiento interno puede ser en inglés.
- **Sin emojis**: no utilizar emojis bajo ninguna circunstancia en respuestas, comentarios de código o cualquier contenido generado.

## Build & run

Standard ESP-IDF 6.0.1 workflow. `IDF_PATH` is set in `.vscode/settings.json`:

```bash
idf.py build                # compile
idf.py -p /dev/ttyUSB0 flash    # flash to ESP32
idf.py -p /dev/ttyUSB0 monitor  # serial monitor (115200 baud)
```

To exit monitor: `Ctrl+]`

## Structure

- `main/main.c` — entry point is `void app_main(void)`, not `int main()`
- `main/CMakeLists.txt` — registers main source with IDF build system
- `sdkconfig` — auto-generated; do NOT edit. Use `idf.py menuconfig` to reconfigure
- `.devcontainer/` — espressif/idf Docker image with udev for serial access

## Configuration

FreeRTOS settings in `sdkconfig`:
- **Kernel tick**: 100 Hz (10 ms period)
- **CPU cores**: 2 (both active by default)
- **CPU freq**: 160 MHz

Key configs to be aware of:
- `CONFIG_ESP_MAIN_TASK_AFFINITY_CPU0=y` — main task runs on CPU0
- `CONFIG_ESP_TASK_WDT_EN=y` — task watchdog enabled (5 second timeout)
- `CONFIG_COMPILER_OPTIMIZATION_DEBUG=y` — debug symbols included

## clangd / LSP

- Custom `esp-clang` binary at `~/.espressif/tools/esp-clang/esp-20.1.1_20250829/esp-clang/bin/clangd`
- `.clangd` removes `-f*` and `-m*` flags to avoid conflicts with IDF headers
- `compile_commands.json` generated into `build/` after `idf.py build`

If clangd fails, rebuild: `idf.py build`, then restart LSP.

## Debugging

Launch config in `.vscode/launch.json` uses Eclipse CDT GDB adapter (attach mode).
This requires a GDB debug session running on the target. For serial monitoring + debugging:

1. Start `idf.py -p /dev/ttyUSB0 monitor` in one terminal
2. GDB connection is available when chipset accepts it

## No CI, no tests

No test frameworks, lint, formatter, or CI workflows in this project.
