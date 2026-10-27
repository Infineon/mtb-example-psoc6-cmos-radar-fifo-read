# Changelog for Code Example for reading the raw radar data from the fifo of XENSIV&trade; BGT60CUTRxx Radar Sensor

## Overview

This code example demonstrates reading the raw radar data from the fifo of the XENSIV&trade; BGT60CUTRxx Radar Sensor.

## What's included?

- Source code for the fifo read example application.
- Driver for the XENSIV&trade; BGT60CUTRxx Radar Sensor.
- SEGGER Real Time Transfer (RTT), vendored under `third-party/segger-rtt/` and redistributed under SEGGER's licence (see `third-party/README.md`).

## What's Changed?

### 1.1.7

#### Add
- Radar frames are streamed to the host as RTT Stream Frames over SEGGER RTT through the debug probe, on RTT channel 1 (`RadarData`, 65536-byte up-buffer, `SEGGER_RTT_MODE_NO_BLOCK_SKIP`), while control and status stay on the UART
- RTT Stream Frame header serializer (`rtt_stream_header.c`): `[START]` magic, little-endian frame number and payload length, 15 bytes in total
- `get_info` reports an `rtt` block with the RTT control block address, the ADC section length in bytes and the acquisition geometry, so a host tool needs no device configuration file of its own
- Radar frames skipped for want of RTT buffer space are counted in the `{"frames_dropped": N}` report; a frame is written whole or not at all, so a drop shows as a gap in the frame numbers rather than a truncated payload
- Measured RTT Stream Frame rate per host documented in the README, guarded against regression by `pytest -m rtt tests/system/test_rtt_frame_rate.py`
- First release of this example as a ModusToolbox&trade; code example. The SEGGER RTT sources ship flattened into `third-party/segger-rtt/`, and the shipped `Makefile` adds that single include path
- Host protocol support: application and sensor metadata registration, `get_info` command handling, and JSON output through the shared protocol layer
- Startup check that one FIFO threshold interrupt delivers exactly one complete radar frame; acquisition does not start on a mismatch
- Lost radar frames reported to the host as `{"frames_dropped": N}`

#### Change
- `stdout` line buffer enlarged to 1024 bytes to hold the `get_info` reply including the `rtt` block in a single write
- `{"frames_dropped": N}` is coalesced: the first drop is reported immediately, then one line per 128 further drops. The count stays cumulative and exact; without this a board with no host attached to the debug probe drops every frame and reports at the acquisition rate, which crowds host commands off the UART
- FIFO status and read access moved to the shared `radar_fifo` driver API instead of example-local helpers
- One complete radar frame is read per interrupt instead of the sample count derived from the FIFO threshold
- A FIFO event that cannot be read no longer halts the application with `CY_ASSERT(0)`

#### Fixed
- `stdout` is line buffered so the example's rare output lines reach the host instead of waiting for the C library buffer to fill
- ADC calibration no longer leaves `SEQ3_DONE` latched in `IR_STAT`; the sensor driver clears it through `IR_CLEAR` once calibration has completed

### 1.1.0

#### Add
- `USE_CASE_NAME` macro used instead of hardcoded application name string

#### Change
- Sensor initialized with explicit `power_mode` (`ACTIVE_MODE`) and `deep_sleep_after_calibration` (`false`) parameters
- FIFO sample count and threshold now printed using `PRIu32` format specifier for portable output

### 0.1.0

This is the initial release of the code example for reading the raw radar data from the fifo of the XENSIV&trade; BGT60CUTRxx Radar Sensor.

#### Add
- Initializes the board, peripherals, and debug UART.
- Configures GPIOs and the SPI interface for radar sensor communication.
- Initializes the radar sensor and sets up the interrupt handler for fifo data ready events.
- Sets up the fifo read configuration.
- Reads the content of the fifo and stores the content internally in a buffer.
- turns on the LED when new fifo data is available and reading is in progress.
- turns off the LED when fifo data reading is completed.


#### Change

#### Fixed

#### Improved

## Supported software and tools

This version of the XENSIV&trade; BGT60CUTRxx 60GHz radar sensor code example was validated for compatibility with the following software and tools:

| Software and tools                        | Version |
| :---                                      | :----:  |
| ModusToolbox&trade; software environment        | 3.1.0   |
| GCC compiler                              | 10.3.1  |

Minimum required ModusToolbox&trade; software environment: v3.2

## More information

Use the following links for more information, as needed:


© Infineon Technologies AG, 2021.
