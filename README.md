# PSOC&trade; 6 MCU: FIFO READ using XENSIV&trade; 60 GHz Radar

This code example demonstrates how to configure and fetch raw radar data from the radar sensor FIFO on the CSK hardware platform, and how to stream those raw ADC samples to a PC through the debug probe using SEGGER's [Real Time Transfer (RTT)](https://kb.segger.com/RTT) interface. It is the starting point for an application that needs the raw ADC samples of a radar frame rather than an algorithm result.

**Platform**: This is the CSK (Connected Sensor Kit) bare-metal version for CYSBSYSKIT-DEV-01. Key features:
- Uses SPI frequency of 18.75 MHz
- DEBUG UART pins for communication
- Streams every radar frame to the host over SEGGER RTT through the debug probe, while control and status stay on the UART
- Describes the stream to the host in the `get_info` response, so a host tool needs no device configuration file of its own
- Includes LED initialization and control
- Green Wing board LED lit while a radar frame is being read
- Reads exactly one complete radar frame per FIFO threshold interrupt
- FIFO integrity checking through the shared `radar_fifo` driver API
- JSON protocol output with host command handling (e.g. `get_info` to query version and sensor info)
- Reports lost radar frames to the host as `{"frames_dropped": N}`, coalesced so a continuously dropping board keeps the UART free for commands
- 5-second UART initialization delay
- Sensor initialized in active mode (`ACTIVE_MODE`) with power down disabled


[View this README on GitHub.](replace_code_example_github_readme_url)

[Provide feedback on this code example.](replace_code_example_voc_survey_url)



## Requirements

- [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) v3.4 or later (tested with v3.4)
- Board support package (BSP) minimum required version: 5.0.0
- Programming language: C
- Associated parts: All [PSOC&trade; 6 MCU](https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-6-32-bit-arm-cortex-m4-mcu) parts



## Supported toolchains (make variable 'TOOLCHAIN')

- GNU Arm&reg; Embedded Compiler v11.3.1 (`GCC_ARM`) – Default value of `TOOLCHAIN`
- Arm&reg; Compiler v6.22 (`ARM`)
- IAR C/C++ Compiler v9.50.2 (`IAR`)




## Supported kits (make variable 'TARGET')

- [Rapid IoT connect developer kit](https://github.com/Infineon/TARGET_CYSBSYSKIT-DEV-01) (`CYSBSYSKIT-DEV-01`) – Default value of `TARGET`

## Hardware setup

### For Rapid IoT Connect Developer Kit

This code example requires the XENSIV&trade; BGT60CUTR13AIP Radar Wing Board as part of the connected sensor kit.

> **Note:** A debug probe is part of the **primary data path** of this example, not just a programming tool. The radar frames leave the target over SEGGER RTT through the probe, so the probe has to stay attached and a host has to keep draining RTT channel 1 for the example to deliver any data at all. The on-board KitProg3 of the CYSBSYSKIT-DEV-01 is enough to carry every frame; whether they all arrive depends on how promptly the PC drains the stream, see [Frame rate](#frame-rate).

1. Connect the radar Wing board to the CYSBSYSKIT-DEV-01 kit through the pin headers.

2. Connect the CYSBSYSKIT-DEV-01 kit to the PC with the USB cable.

   **Figure 1. Rapid IoT Connect Developer Kit**

   ![](images/featherkit.png)


3. Place the CYSBSYSKIT-DEV-01 kit at a fixed location (for example, the corner of a room) to ensure optimal performance of the presence detection application.




## Software setup

See the [ModusToolbox&trade; tools package installation guide](https://www.infineon.com/ModusToolboxInstallguide) for information about installing and configuring the tools package.


Install a terminal emulator if you do not have one. Instructions in this document use [Tera Term](https://teratermproject.github.io/index-en.html).

This example requires no additional software or tools.



## Using the code example


### Create the project

The ModusToolbox&trade; tools package provides the Project Creator as both a GUI tool and a command line tool.

<details><summary><b>Use Project Creator GUI</b></summary>

1. Open the Project Creator GUI tool

   There are several ways to do this, including launching it from the dashboard or from inside the Eclipse IDE. For more details, see the [Project Creator user guide](https://www.infineon.com/ModusToolboxProjectCreator) (locally available at *{ModusToolbox&trade; install directory}/tools_{version}/project-creator/docs/project-creator.pdf*)

2. On the **Choose Board Support Package (BSP)** page, select a kit supported by this code example. See [Supported kits](#supported-kits-make-variable-target)

   > **Note:** To use this code example for a kit not listed here, you may need to update the source files. If the kit does not have the required resources, the application may not work

3. On the **Select Application** page:

   a. Select the **Applications(s) Root Path** and the **Target IDE**

      > **Note:** Depending on how you open the Project Creator tool, these fields may be pre-selected for you

   b. Select this code example from the list by enabling its check box

      > **Note:** You can narrow the list of displayed examples by typing in the filter box

   c. (Optional) Change the suggested **New Application Name** and **New BSP Name**

   d. Click **Create** to complete the application creation process

</details>


<details><summary><b>Use Project Creator CLI</b></summary>

The 'project-creator-cli' tool can be used to create applications from a CLI terminal or from within batch files or shell scripts. This tool is available in the *{ModusToolbox&trade; install directory}/tools_{version}/project-creator/* directory.

Use a CLI terminal to invoke the 'project-creator-cli' tool. On Windows, use the command-line 'modus-shell' program provided in the ModusToolbox&trade; installation instead of a standard Windows command-line application. This shell provides access to all ModusToolbox&trade; tools. You can access it by typing "modus-shell" in the search box in the Windows menu. In Linux and macOS, you can use any terminal application.

The following example clones the "[FIFO Read](https://github.com/Infineon/mtb-example-psoc6-cmos-radar-fifo-read)" application with the desired name "my-fifo-read" configured for the *CYSBSYSKIT-DEV-01* BSP into the specified working directory, *./mtb_projects*:

   ```
   project-creator-cli --board-id CYSBSYSKIT-DEV-01 --app-id mtb-example-psoc6-cmos-radar-fifo-read --user-app-name my-fifo-read --target-dir ./mtb_projects
   ```

The 'project-creator-cli' tool has the following arguments:

Argument | Description | Required/optional
---------|-------------|-----------
`--board-id` | Defined in the <id> field of the [BSP](https://github.com/Infineon?q=bsp-manifest&type=&language=&sort=) manifest | Required
`--app-id`   | Defined in the <id> field of the [CE](https://github.com/Infineon?q=ce-manifest&type=&language=&sort=) manifest | Required
`--target-dir`| Specify the directory in which the application is to be created if you prefer not to use the default current working directory | Optional
`--user-app-name`| Specify the name of the application if you prefer to have a name other than the example's default name | Optional

<br>

> **Note:** The project-creator-cli tool uses the `git clone` and `make getlibs` commands to fetch the repository and import the required libraries. For details, see the "Project creator tools" section of the [ModusToolbox&trade; tools package user guide](https://www.infineon.com/ModusToolboxUserGuide) (locally available at {ModusToolbox&trade; install directory}/docs_{version}/mtb_user_guide.pdf).

</details>



### Open the project

After the project has been created, you can open it in your preferred development environment.


<details><summary><b>Eclipse IDE</b></summary>

If you opened the Project Creator tool from the included Eclipse IDE, the project will open in Eclipse automatically.

For more details, see the [Eclipse IDE for ModusToolbox&trade; user guide](https://www.infineon.com/MTBEclipseIDEUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_ide_user_guide.pdf*).

</details>


<details><summary><b>Visual Studio (VS) Code</b></summary>

Launch VS Code manually, and then open the generated *{project-name}.code-workspace* file located in the project directory.

For more details, see the [Visual Studio Code for ModusToolbox&trade; user guide](https://www.infineon.com/MTBVSCodeUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_vscode_user_guide.pdf*).

</details>


<details><summary><b>Arm&reg; Keil&reg; µVision&reg;</b></summary>

Double-click the generated *{project-name}.cprj* file to launch the Keil&reg; µVision&reg; IDE.

For more details, see the [Arm&reg; Keil&reg; µVision&reg; for ModusToolbox&trade; user guide](https://www.infineon.com/MTBuVisionUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_uvision_user_guide.pdf*).

</details>


<details><summary><b>IAR Embedded Workbench</b></summary>

Open IAR Embedded Workbench manually, and create a new project. Then select the generated *{project-name}.ipcf* file located in the project directory.

For more details, see the [IAR Embedded Workbench for ModusToolbox&trade; user guide](https://www.infineon.com/MTBIARUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_iar_user_guide.pdf*).

</details>


<details><summary><b>Command line</b></summary>

If you prefer to use the CLI, open the appropriate terminal, and navigate to the project directory. On Windows, use the command-line 'modus-shell' program; on Linux and macOS, you can use any terminal application. From there, you can run various `make` commands.

For more details, see the [ModusToolbox&trade; tools package user guide](https://www.infineon.com/ModusToolboxUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mtb_user_guide.pdf*).

</details>



## Operation


If using a PSOC&trade; 64 "Secure" MCU kit (like CY8CKIT-064B0S2-4343W), the PSOC&trade; 64 device must be provisioned with keys and policies before being programmed. Follow the instructions in the ["Secure Boot" SDK user guide](https://www.infineon.com/dgdlac/Infineon-PSoC_64_Secure_MCU_Secure_Boot_SDK_User_Guide-Software-v07_00-EN.pdf?fileId=8ac78c8c7d0d8da4017d0f8c361a7666) to provision the device. If the kit is already provisioned, copy-paste the keys and policy folder to the application folder.

1. Connect the board to your PC using the provided USB cable through the KitProg3 USB connector

2. Open a terminal program and select the KitProg3 COM port. Set the serial port parameters to 8N1 and 2000000 baud

3. Program the board using one of the following:

   <details><summary><b>Using Eclipse IDE</b></summary>

      1. Select the application project in the Project Explorer

      2. In the **Quick Panel**, scroll down, and click **\<Application Name> Program (KitProg3_MiniProg4)**
   </details>


   <details><summary><b>In other IDEs</b></summary>

   Follow the instructions in your preferred IDE

   </details>


   <details><summary><b>Using CLI</b></summary>

     From the terminal, execute the `make program` command to build and program the application using the default toolchain to the default target. The default toolchain is specified in the application's Makefile but you can override this value manually:
      ```
      make program TOOLCHAIN=<toolchain>
      ```

      Example:
      ```
      make program TOOLCHAIN=GCC_ARM
      ```
   </details>

4. After programming, the application starts automatically. Confirm that "\<CE Title>" is displayed on the UART terminal


   **Terminal output on program startup**

   Every line the application prints is a JSON object, so the output can be parsed by host tooling without screen scraping:

   ```
   {"info":"******************"}
   {"info":"Radar: FIFO Read Example"}
   {"info":"******************"}
   {"info":"For more projects, visit our code examples repositories:"}
   {"info":"https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software"}
   {"info":"Radar sensor is initialized"}
   {"info":"Radar frame size: 3264 samples"}
   {"info":"FIFO IRQ threshold: 3264 samples"}
   {"info":"Radar data acquisition started"}
   ```

   The UART is quiet afterwards: every radar frame goes out over RTT, not over the terminal. Lines only appear when something happens that the host needs to know about — a lost radar frame, a failed FIFO access, or a reply to a host command:

   ```
   {"info":"FIFO overflow, radar frame discarded"}
   {"frames_dropped": 1}
   ```

   > **Note:** Nothing drains the RTT stream until a host attaches to the debug probe. The up-buffer holds ten radar frames, so with no host attached the example reports a rising `frames_dropped` count, one line per 128 further drops. This is the expected idle behavior, not a fault. See [Streaming radar frames over RTT](#streaming-radar-frames-over-rtt).


5. Confirm that the green LED on the radar Wing board lights up briefly for every radar frame that is read.


## Debugging

You can debug the example to step through the code.


<details><summary><b>In Eclipse IDE</b></summary>

Use the **\<Application Name> Debug (KitProg3_MiniProg4)** configuration in the **Quick Panel**. For details, see the "Program and debug" section in the [Eclipse IDE for ModusToolbox&trade; user guide](https://www.infineon.com/MTBEclipseIDEUserGuide).

> **Note:** **(Only while debugging)** On the CM4 CPU, some code in `main()` may execute before the debugger halts at the beginning of `main()`. This means that some code executes twice – once before the debugger stops execution, and again after the debugger resets the program counter to the beginning of `main()`. See [KBA231071](https://community.infineon.com/docs/DOC-21143) to learn about this and for the workaround.

</details>


<details><summary><b>In other IDEs</b></summary>

Follow the instructions in your preferred IDE.

</details>


## Design and implementation

The application configures the sensor from the register list in *radar_config/*, enables the FIFO threshold interrupt, and then serves three things in its main loop: the host protocol, the FIFO, and the RTT stream.

### One radar frame per interrupt

A radar frame is `SEQ0_RX_ANTENNAS * SEQ0_NUMBER_OF_CHIRPS * SEQ0_SAMPLES_PER_CHIRP` samples. The FIFO threshold interrupt fires one FIFO word past `FIFO_THRESHOLD`, and each word carries two samples, so an interrupt delivers `2 * (FIFO_THRESHOLD + 1)` samples. The example reads a whole radar frame per interrupt, which is only correct while those two numbers are equal.

That equality is a property of the register configuration, not of the code, so the application prints both numbers at startup and checks them before acquisition begins. On a mismatch it prints an error naming both values and returns without starting the sequence, rather than streaming frames that are silently misaligned to the acquisition boundary — a misalignment the host cannot detect from the data. Changing the radar configuration therefore requires `FIFO_THRESHOLD` to be adjusted with it.

### Host protocol

Output goes through the shared protocol layer in *common-src/* instead of bare `printf()`, so every line is a JSON object. The application registers its name, version and sensor metadata at startup and calls `protocol_handler()` on every loop iteration, which answers the `get_info` command from the PC. A radar frame counter is passed as the timestamp.

newlib leaves `stdout` fully buffered because the retarget-io backend has no `fstat()`, so the example switches it to line buffering with a statically supplied 1024-byte buffer once retarget-io is up. Line buffering flushes every newline-terminated protocol message immediately while still handing whole lines to the UART; the buffer is sized for the longest line the example emits, which is the `get_info` reply including the `rtt` block.

### FIFO handling and frame loss

FIFO access uses the shared `radar_fifo` driver API (*sensor-xensiv-bgt60cutrxx/radar_fifo/*). Each interrupt reads the FIFO status first: if the sensor reports an overflow, the radar frame is discarded, a running counter of lost frames is reported to the host as `{"frames_dropped": N}`, and acquisition continues. There is no FIFO drain or sequence restart, because the FIFO is emptied completely on every interrupt and there is no partially consumed frame to resynchronize from.

The same counter also covers frames lost to RTT back-pressure, described below.

The counter is cumulative and always exact, but it is not printed on every drop. The first drop is reported immediately, and after that one line is printed per 128 further drops. A board that drops every frame — the normal state with no host attached — would otherwise emit a line at the full acquisition rate, which keeps the UART busy enough that host commands are missed and the `get_info` reply never gets out.

### Streaming radar frames over RTT

One radar frame of the shipped configuration is 6528 bytes of payload. The 2 Mbaud terminal link cannot carry that at the rate the sensor produces it, so the UART is used for control and status only and the radar frames leave the target over SEGGER RTT through the debug probe.

The transport is:

- SEGGER RTT, vendored at *third-party/segger-rtt/* and described with its licence in *third-party/README.md*
- an up-buffer on channel **1**, named `RadarData`, 65536 bytes — ten radar frames of the shipped configuration. Channel 0 is left as SEGGER's terminal
- mode `SEGGER_RTT_MODE_NO_BLOCK_SKIP`, so the target never stalls waiting for the host
- a host that attaches a debug probe (for example OpenOCD or J-Link) and drains channel 1

Each radar frame is written as one RTT Stream Frame: a fixed 15-byte header followed by the payload.

**Table 1. RTT Stream Frame layout**

 Field | Size | Contents
 :----- | :---- | :--------
 Magic | 7 bytes | `[START]`
 Frame number | 4 bytes | `uint32_t` little-endian, monotonic, wraps at `UINT32_MAX`
 Payload length | 4 bytes | `uint32_t` little-endian, size of the payload in bytes
 Payload | payload length | Raw ADC samples, `uint16_t` little-endian

<br>

For the shipped configuration that is 7 + 4 + 4 + 6528 = **6543 bytes** per radar frame.

The payload is the frame buffer verbatim: the samples keep the FIFO's native interleaving, with the receive-antenna index changing fastest, then the sample index, then the chirp index. The target performs no reordering — reshaping is the host's job. The payload length field is load-bearing rather than advisory: a host that finds it disagreeing with the geometry it expects has lost sync and should resynchronize on the magic.

The header is serialized by *rtt_stream_header.c*, which is plain byte manipulation with no SEGGER dependency and is unit tested on the host in *tests/rtt-stream/*. The channel setup and the two RTT writes live in *rtt_stream.c*.

#### Frame loss over RTT

A radar frame is written whole or not at all. Before writing, the application checks that the up-buffer can take header and payload together and skips the entire frame if it cannot, so the host never sees a truncated payload — it sees a gap in the frame numbers instead. The frame number is the acquisition counter rather than a count of transmitted frames, so that gap independently corroborates the `{"frames_dropped": N}` count the application reports on the UART.

Relying on the skip behavior of `SEGGER_RTT_Write()` alone would not do: it would drop the payload but keep the header, and desynchronize the host.

With no host draining channel 1 the up-buffer fills after ten frames and every frame after that is dropped, so a board left running on its own reports a steadily rising `frames_dropped` count, coalesced as described in [FIFO handling and frame loss](#fifo-handling-and-frame-loss).

#### Frame rate

The sensor acquires 20.0 radar frames per second in the shipped configuration, which is 6543 bytes per frame or roughly 1.05 Mbps. Whether all of them reach the PC is set by how promptly the host drains the stream, not by the sensor.

The numbers below hold only for `BGT60CUTR13B_80MhzXtal_450MHzeff_64nsa_17chp`, since both the frame size and the acquisition rate follow from it.

**Table 2. RTT Stream Frame rate at the host**

 Host | Probe | SWD speed | Frame rate
 :--- | :---- | :-------- | :----------
 Windows 11, MS Defender and CrowdStrike active | KitProg3 (on board) | 2000 kHz | 16.2 frames/s of 20.0 acquired
 macOS 26 | KitProg3 (on board) | 2000 kHz | No frame loss observed
 — | SEGGER J-Link Plus | 16000 kHz | Not measured

<br>

**The host is the bottleneck, not the probe.** The same board and the same on-board KitProg3 lose nothing on macOS but drop about 4 of every 20 frames on a Windows machine running endpoint security software. Lost frames show as gaps in the frame numbers and in the `{"frames_dropped": N}` count, so check what else runs on the PC before reaching for a faster probe. Both rows are single-machine observations, not a KitProg3 throughput specification.

The Windows row is measured by *tests/system/test_rtt_frame_rate.py*, which drains RTT channel 1 through OpenOCD's RTT TCP server for ten seconds and counts complete RTT Stream Frames. Guarded by the opt-in `rtt` marker, it asserts a floor of 12.2 frames/s — about 75 % of that rate — to catch a firmware regression without failing on host variation:

```
pytest -m mtb tests/system/test_apps.py -k fifo-read-csk
pytest -m rtt tests/system/test_rtt_frame_rate.py -s
```

Flash the board with the first command before measuring with the second, so the rate belongs to the firmware under test.

#### The `rtt` block in the `get_info` response

A host needs two things before it can record: the address of the RTT control block, so it can attach without searching target memory, and the acquisition geometry, so it can reshape the payload. The application registers both at startup and the `get_info` reply carries them in an `rtt` object:

```json
"rtt": {
  "addr": "0x08001234",
  "data_and_payload_length_bytes": {"adc": 6528, "rfft": 0},
  "msb_aligned": false,
  "radar_config": {"num_tx": 1, "num_rx": 3, "num_chirps": 17, "num_samples_per_chirp": 64, "range_resolution_m": 0.000000}
}
```

`data_and_payload_length_bytes` is an ordered map: the key order is the on-wire section order and a length of `0` marks a section as disabled. This example emits an `adc` section only, so its `rfft` section is reported as zero length. `radar_config` is authoritative for the host, which is what removes the need for a device configuration file on the PC side. Its `range_resolution_m` is `0` here, because the register configuration of this example does not name a range resolution; the sibling `fifo-read-rfft` example reports the spacing of its range FFT bins.

`msb_aligned` reports `SPI_CTRL.MSB_ALIGN_EN` as the sensor is actually configured, which tells the host where a sample sits inside its transfer word — here, whether the 12-bit ADC sample occupies the high or the low bits of its 16-bit slot. It is read back from the sensor rather than hard-coded, because it follows from the register configuration and differs between the code examples in this repository.

The block is emitted only by an application that registered one, so the `get_info` response of every other code example in this repository is unchanged.

> **Note:** The register configuration header names the receive antenna count, the chirp count and the samples per chirp, but not the transmit antenna count. `num_tx` is therefore fixed at 1 in the application, matching the shipped sequence, and has to be revisited if the configuration is changed to transmit on more than one antenna.

### Resources and settings

**Table 3. Application resources**

 Resource  |  Alias/object     |    Purpose
 :-------- | :-------------    | :------------
 UART (HAL)|cy_retarget_io_uart_obj| UART HAL object used by Retarget-IO for the Debug UART port
 GPIO (HAL)    | CYBSP_USER_LED     | User LED
 GPIO (HAL)    | SHIELD_XENSIV_RADAR_LED_GREEN | Lit while a radar frame is read from the FIFO
 GPIO (HAL)    | SHIELD_XENSIV_RADAR_LED_RED | Wing board status LED

<br>



## Related resources

Resources  | Links
-----------|----------------------------------
Application notes  | [AN228571](https://www.infineon.com/AN228571) – Getting started with PSOC&trade; 6 MCU on ModusToolbox&trade; <br>  [AN215656](https://www.infineon.com/AN215656) – PSOC&trade; 6 MCU: Dual-CPU system design <br> [AN79953](https://www.infineon.com/AN79953) – Getting started with PSOC&trade; 4 MCU <br>  [AN85951](https://www.infineon.com/AN85951) – PSOC&trade; 4 and PSOC&trade; 6 MCU CAPSENSE&trade; design guide
Code examples  | [Using ModusToolbox&trade;](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software) on GitHub
Device documentation | [PSOC&trade; 6 MCU datasheets](https://documentation.infineon.com/html/psoc6/bnm1651211483724.html) <br> [PSOC&trade; 6 technical reference manuals](https://documentation.infineon.com/html/psoc6/zrs1651212645947.html)<br> [PSOC&trade; 4 datasheets](https://www.infineon.com/cms/en/search.html?intc=searchkwr-return#!view=downloads&term=psoc%204&doc_group=Data%20Sheet) <br>[PSOC&trade; 4 technical reference manuals](https://www.infineon.com/cms/en/search.html#!term=psoc%204%20technical%20reference%20manual&view=all)
Development kits | Select your kits from the [Evaluation board finder](https://www.infineon.com/cms/en/design-support/finder-selection-tools/product-finder/evaluation-board)
Libraries on GitHub  | [mtb-pdl-cat1](https://github.com/Infineon/mtb-pdl-cat1) – PSOC&trade; 6 Peripheral Driver Library (PDL)  <br> [mtb-hal-cat1](https://github.com/Infineon/mtb-hal-cat1) – Hardware Abstraction Layer (HAL) library <br> [retarget-io](https://github.com/Infineon/retarget-io) – Utility library to retarget STDIO messages to a UART port <br>  [mtb-pdl-cat2](https://github.com/Infineon/mtb-pdl-cat2) – PSOC&trade; 4 Peripheral Driver Library (PDL) <br>  [mtb-hal-cat2](https://github.com/Infineon/mtb-hal-cat2) – Hardware Abstraction Layer (HAL) library
Middleware on GitHub  | [capsense](https://github.com/Infineon/capsense) – CAPSENSE&trade; library and documents <br> [psoc6-middleware](https://github.com/Infineon/modustoolbox-software#psoc-6-middleware-libraries) – Links to all PSOC&trade; 6 MCU middleware
Tools  | [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) – ModusToolbox&trade; software is a collection of easy-to-use libraries and tools enabling rapid development with Infineon MCUs for applications ranging from wireless and cloud-connected systems, edge AI/ML, embedded sense and control, to wired USB connectivity using PSOC&trade; Industrial/IoT MCUs, AIROC&trade; Wi-Fi and Bluetooth&reg; connectivity devices, XMC&trade; Industrial MCUs, and EZ-USB&trade;/EZ-PD&trade; wired connectivity controllers. ModusToolbox&trade; incorporates a comprehensive set of BSPs, HAL, libraries, configuration tools, and provides support for industry-standard IDEs to fast-track your embedded application development

<br>


## Other resources


Infineon provides a wealth of data at [www.infineon.com](https://www.infineon.com) to help you select the right device, and quickly and effectively integrate it into your design.

For more information about Connected Sensor Kit, see [IoT Sensors Platform](https://www.infineon.com/connectedsensorkit).

## Document history


Document title: *CEnnnnn* – *Title of this document*

 Version | Description of change
 ------- | ---------------------
 Unreleased | Radar frames streamed to the host as RTT Stream Frames over SEGGER RTT, on a dedicated channel with a skip-if-full policy<br>`get_info` reports the RTT control block address, the ADC section length and the acquisition geometry in an `rtt` block<br>Output converted to the shared JSON protocol layer with host command handling (`get_info`)<br>Reads one complete radar frame per FIFO threshold interrupt and verifies the threshold against the radar frame size at startup<br>FIFO access moved to the shared `radar_fifo` driver API<br>Frames lost to RTT back-pressure reported as `{"frames_dropped": N}`
 1.1.0   | Added `power_mode` and `deep_sleep_after_calibration` parameters to sensor init (active mode default)
 0.5.0   | New code example

<br>



All referenced product or service names and trademarks are the property of their respective owners.

The Bluetooth&reg; word mark and logos are registered trademarks owned by Bluetooth SIG, Inc., and any use of such marks by Infineon is under license.

PSOC&trade;, formerly known as PSoC&trade;, is a trademark of Infineon Technologies. Any references to PSoC&trade; in this document or others shall be deemed to refer to PSOC&trade;.

---------------------------------------------------------

© Cypress Semiconductor Corporation, 2020-2026. This document is the property of Cypress Semiconductor Corporation, an Infineon Technologies company, and its affiliates ("Cypress").  This document, including any software or firmware included or referenced in this document ("Software"), is owned by Cypress under the intellectual property laws and treaties of the United States and other countries worldwide.  Cypress reserves all rights under such laws and treaties and does not, except as specifically stated in this paragraph, grant any license under its patents, copyrights, trademarks, or other intellectual property rights.  If the Software is not accompanied by a license agreement and you do not otherwise have a written agreement with Cypress governing the use of the Software, then Cypress hereby grants you a personal, non-exclusive, nontransferable license (without the right to sublicense) (1) under its copyright rights in the Software (a) for Software provided in source code form, to modify and reproduce the Software solely for use with Cypress hardware products, only internally within your organization, and (b) to distribute the Software in binary code form externally to end users (either directly or indirectly through resellers and distributors), solely for use on Cypress hardware product units, and (2) under those claims of Cypress's patents that are infringed by the Software (as provided by Cypress, unmodified) to make, use, distribute, and import the Software solely for use with Cypress hardware products.  Any other use, reproduction, modification, translation, or compilation of the Software is prohibited.
<br>
TO THE EXTENT PERMITTED BY APPLICABLE LAW, CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS DOCUMENT OR ANY SOFTWARE OR ACCOMPANYING HARDWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  No computing device can be absolutely secure.  Therefore, despite security measures implemented in Cypress hardware or software products, Cypress shall have no liability arising out of any security breach, such as unauthorized access to or use of a Cypress product. CYPRESS DOES NOT REPRESENT, WARRANT, OR GUARANTEE THAT CYPRESS PRODUCTS, OR SYSTEMS CREATED USING CYPRESS PRODUCTS, WILL BE FREE FROM CORRUPTION, ATTACK, VIRUSES, INTERFERENCE, HACKING, DATA LOSS OR THEFT, OR OTHER SECURITY INTRUSION (collectively, "Security Breach").  Cypress disclaims any liability relating to any Security Breach, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any Security Breach.  In addition, the products described in these materials may contain design defects or errors known as errata which may cause the product to deviate from published specifications. To the extent permitted by applicable law, Cypress reserves the right to make changes to this document without further notice. Cypress does not assume any liability arising out of the application or use of any product or circuit described in this document. Any information provided in this document, including any sample design information or programming code, is provided only for reference purposes.  It is the responsibility of the user of this document to properly design, program, and test the functionality and safety of any application made of this information and any resulting product.  "High-Risk Device" means any device or system whose failure could cause personal injury, death, or property damage.  Examples of High-Risk Devices are weapons, nuclear installations, surgical implants, and other medical devices.  "Critical Component" means any component of a High-Risk Device whose failure to perform can be reasonably expected to cause, directly or indirectly, the failure of the High-Risk Device, or to affect its safety or effectiveness.  Cypress is not liable, in whole or in part, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any use of a Cypress product as a Critical Component in a High-Risk Device. You shall indemnify and hold Cypress, including its affiliates, and its directors, officers, employees, agents, distributors, and assigns harmless from and against all claims, costs, damages, and expenses, arising out of any claim, including claims for product liability, personal injury or death, or property damage arising from any use of a Cypress product as a Critical Component in a High-Risk Device. Cypress products are not intended or authorized for use as a Critical Component in any High-Risk Device except to the limited extent that (i) Cypress's published data sheet for the product explicitly states Cypress has qualified the product for use in a specific High-Risk Device, or (ii) Cypress has given you advance written authorization to use the product as a Critical Component in the specific High-Risk Device and you have signed a separate indemnification agreement.
<br>
Cypress, the Cypress logo, and combinations thereof, ModusToolbox, PSoC, CAPSENSE, EZ-USB, F-RAM, and TRAVEO are trademarks or registered trademarks of Cypress or a subsidiary of Cypress in the United States or in other countries. For a more complete list of Cypress trademarks, visit www.infineon.com. Other names and brands may be claimed as property of their respective owners.
