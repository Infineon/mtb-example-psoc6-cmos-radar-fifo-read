/*
 * main.c
 *
 * Description: Demonstrates reading raw radar data frames from the FIFO buffer of the XENSIV BGT60CUTRxx sensor.
 *
 * Related Document: See README.md
 *
 ********************************************************************************
 * Copyright (c) (2016-2022), Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#include "cyhal.h"
#include "cy_retarget_io.h"
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include "common.h"
#include "protocol.h"
#include "resource_map.h"
#include "radar_config/BGT60CUTR13B_80MhzXtal_450MHzeff_64nsa_17chp.h"
#include "radar_fifo_read.h"
#include "rtt_stream.h"
#include "version.h"
#include "xensiv_bgt60cutrxx_mtb.h"
#include "xensiv_bgt60cutrxx_regs.h"

/*******************************************************************************
* Macros
*******************************************************************************/

/** baud rate for the UART communication
 * The baudrate must be high enough to transmit the data before the next interrupt occurs.
 */
#define UART_BAUDRATE 2000000

/** Default SPI speed for the XENSIV BGT60CUTRxx sensor, which is used during startup */
#define SPI_FREQUENCY               (18750000UL) // 18.75MHz

/** radar frame size: receive antennas * chirps * samples per chirp. */
#define NUM_FRAME_SAMPLES (SEQ0_RX_ANTENNAS * SEQ0_NUMBER_OF_CHIRPS * SEQ0_SAMPLES_PER_CHIRP)

/** The shipped sequence transmits on a single antenna; the register configuration does not name it. */
#define NUM_TX_ANTENNAS (1U)

/** FIFO threshold IRQ fires one word past the threshold; each word holds two samples. */
#define NUM_THRESHOLD_SAMPLES (2U * (FIFO_THRESHOLD + 1U))

// Enable this newlib workaround only for GCC: it flushes protocol lines promptly. IAR lacks
// these stdio identifiers, so disabling it there keeps the build valid but uses default buffering.
#if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__IAR_SYSTEMS_ICC__)
/** Holds the longest protocol line (the get_info reply with the rtt block, ~560 bytes) so it is
 * written in one go. */
#define STDOUT_BUFFER_SIZE (1024U)
#endif

/** Dropped frames are reported once, then once per this many further drops.
 *
 * Nothing drains the RTT up-buffer until a host attaches to the debug probe, so an unattended
 * board drops every frame at the acquisition rate. One line per drop keeps the UART busy enough
 * that incoming commands are missed and the get_info reply never gets out. */
#define FRAMES_DROPPED_REPORT_INTERVAL (128U)

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void xensiv_bgt60cutrxx_irq_handler(void* callback_arg, cyhal_gpio_event_t event);
static int32_t xensiv_bgt60cutrxx_board_init(void);
static int32_t initialize_board_leds(void);
static void report_frames_dropped(uint32_t frames_dropped);

/*******************************************************************************
* Global Variables
*******************************************************************************/

/** Radar sensor object */
static xensiv_bgt60cutrxx_mtb_t sensor;

/**
 * @brief Data buffer used to store one complete radar frame.
 *
 * The size of one radar frame element is 16 bits.
 */
static uint16_t frame_data_buffer[NUM_FRAME_SAMPLES];


/** Helper variable used by IRQ handler to indicate data events to main loop */
static volatile bool radar_event_irq = false;

#if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__IAR_SYSTEMS_ICC__)
/** Line buffer for stdout, supplied statically to keep stdio off the heap */
static char stdout_buffer[STDOUT_BUFFER_SIZE];
#endif

/** Up-buffer memory for the RTT stream, owned by SEGGER RTT from rtt_stream_init() onwards */
static uint8_t rtt_stream_buffer[RTT_STREAM_BUFFER_SIZE];

/** Description of the RTT stream; the protocol handler keeps a reference to it */
static protocol_rtt_info_t rtt_info =
{
    .adc_length_bytes          = NUM_FRAME_SAMPLES * sizeof(uint16_t),
    .rfft_length_bytes         = 0U,
    .radar_config              =
    {
        .num_tx                = NUM_TX_ANTENNAS,
        .num_rx                = SEQ0_RX_ANTENNAS,
        .num_chirps            = SEQ0_NUMBER_OF_CHIRPS,
        .num_samples_per_chirp = SEQ0_SAMPLES_PER_CHIRP
    }
};

/** Platform interface configuration */
static xensiv_bgt60cutrxx_mtb_iface_config_t mcu_iface_config =
{
    .selpin       = PIN_XENSIV_BGT60CUTRXX_SPI_CSN,
    .rstpin       = PIN_XENSIV_BGT60CUTRXX_RSTN,
    .irqpin       = PIN_XENSIV_BGT60CUTRXX_IRQ,
    .irq_priority = CYHAL_ISR_PRIORITY_DEFAULT,
    .spi_mosi     = CYBSP_SPI_MOSI,
    .spi_miso     = CYBSP_SPI_MISO,
    .spi_clk      = CYBSP_SPI_CLK,
    .spi_speed_hz = SPI_FREQUENCY,
};

/*******************************************************************************
* Function Definitions
*******************************************************************************/

/**
 * @brief Interrupt handler for the XENSIV BGT60CUTRxx radar sensor.
 *
 * This function is called when a radar interrupt occurs. It sets the radar_event_irq
 * flag to signal the main loop that radar data is ready for processing.
 *
 * @param[in] callback_arg User-defined callback argument (unused).
 * @param[in] event GPIO event that triggered the interrupt.
 */
static void xensiv_bgt60cutrxx_irq_handler(void* callback_arg, cyhal_gpio_event_t event)
{
    (void)callback_arg;
    (void)event;

    // flag to indicate radar event
    radar_event_irq = true;
}


/** Helper function to initialize board specific settings */
static int32_t xensiv_bgt60cutrxx_board_init(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Enable LDO */
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_LDO_EN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    /* Set specific pins to disable BGT60CUTRxx bootloader */
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_DIO2, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    result |= cyhal_gpio_init(PIN_XENSIV_BGT60CUTRXX_DIO3, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    /* Set level shifter direction */
    result |= cyhal_gpio_init(SHIELD_XENSIV_DIR4_EN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    if (result != CY_RSLT_SUCCESS)
    {
        return -1;
    }

    return XENSIV_BGT60CUTRXX_STATUS_OK;
}


/** Helper function to initialize board LEDs */
static int32_t initialize_board_leds(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    result |= cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
    result |= cyhal_gpio_init(SHIELD_XENSIV_RADAR_LED_RED, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, SHIELD_XENSIV_RADAR_LED_RED_OFF);
    result |= cyhal_gpio_init(SHIELD_XENSIV_RADAR_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT,
                              CYHAL_GPIO_DRIVE_STRONG, SHIELD_XENSIV_RADAR_LED_GREEN_OFF);

    if (result != CY_RSLT_SUCCESS)
    {
        return -1;
    }

    return XENSIV_BGT60CUTRXX_STATUS_OK;
}


/**
 * @brief Reports lost radar frames to the host, coalesced to keep the control link usable.
 *
 * The first drop is reported immediately so frame loss is never silent, after which the report
 * repeats every FRAMES_DROPPED_REPORT_INTERVAL drops. The reported count is always the
 * cumulative total, so no drop is hidden, only the line that would have announced it.
 *
 * @param frames_dropped Cumulative number of radar frames that could not be delivered.
 */
static void report_frames_dropped(uint32_t frames_dropped)
{
    if ((frames_dropped == 1U) || ((frames_dropped % FRAMES_DROPPED_REPORT_INTERVAL) == 0U))
    {
        protocol_print_frames_dropped(frames_dropped);
    }
}


/**
 * @brief Main application entry point for radar FIFO data reading.
 *
 * This function performs the following operations:
 * - Initializes the device, board peripherals, and UART for debug output
 * - Configures GPIO pins and SPI interface for the XENSIV BGT60CUTRxx radar sensor
 * - Sets up the radar sensor with interrupt-driven data acquisition
 * - Verifies that one FIFO threshold interrupt delivers exactly one complete radar frame
 * - Enters an infinite loop serving the host protocol, reading one complete radar frame per
 *   interrupt, and controlling the green LED to indicate processing activity
 *
 * @return int This function never returns under normal operation due to infinite loop.
 */
int main(void)
{
    cy_rslt_t result;

    #if defined (CY_DEVICE_SECURE)
    cyhal_wdt_t wdt_obj;

    /* Clear watchdog timer so that it doesn't trigger a reset */
    result = cyhal_wdt_init(&wdt_obj, cyhal_wdt_get_max_timeout_ms());
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    cyhal_wdt_free(&wdt_obj);
    #endif /* #if defined (CY_DEVICE_SECURE) */

    // Initialize MCU and board peripherals
    result = cybsp_init();
    result |= initialize_board_leds();
    if (result != CY_RSLT_SUCCESS)
    {
        // MCU and board init failed. Stop program execution
        CY_ASSERT(0);
        return 1;
    }

    // Enable global interrupts
    __enable_irq();

    // Initialize UART port used for logging
    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, NC, NC, UART_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        // retarget-io init failed. Stop program execution
        CY_ASSERT(0);
        return 1;
    }

    // wait until UART is ready
    cyhal_system_delay_ms(5000);

    // Enable this newlib workaround only for GCC: it flushes protocol lines promptly. IAR lacks
    // these stdio identifiers, so disabling it there keeps the build valid but uses default buffering.
    #if defined(__GNUC__) && !defined(__ARMCC_VERSION) && !defined(__IAR_SYSTEMS_ICC__)
    // newlib leaves stdout fully buffered because the retarget-io backend has no fstat();
    // this example prints too rarely to ever fill that buffer, so its lines would never reach the host.
    // Every protocol message is newline terminated, so line buffering flushes each one while still
    // handing whole lines to the UART rather than one character per write.
    setvbuf(stdout, stdout_buffer, _IOLBF, sizeof(stdout_buffer));
    #endif

    protocol_print_info("******************");
    protocol_print_info("Radar: FIFO Read Example");
    protocol_print_info("******************");

    protocol_print_info("For more projects, "
                        "visit our code examples repositories:");

    protocol_print_info("https://github.com/Infineon/"
                        "Code-Examples-for-ModusToolbox-Software");

    result = xensiv_bgt60cutrxx_board_init();
    result |= xensiv_bgt60cutrxx_platform_open(&sensor.iface, &mcu_iface_config);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Could not initialize the platform interfaces");
        return 1;
    }

    // Radar sensor initialization
    xensiv_bgt60cutrxx_power_mode_t power_mode = ACTIVE_MODE;
    bool deep_sleep_after_calibration = false;
    result = xensiv_bgt60cutrxx_platform_init(&sensor, &mcu_iface_config, register_list, ARRAY_SIZE(
                                                  register_list), power_mode, deep_sleep_after_calibration);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Could not initialize the radar sensor");
        return 1;
    }
    protocol_print_info("Radar sensor is initialized");

    result = xensiv_bgt60cutrxx_platform_irq_init(&sensor, &mcu_iface_config, &xensiv_bgt60cutrxx_irq_handler, NULL);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Could not initialize the radar IRQ handler");
        return 1;
    }

    result = xensiv_bgt60cutrxx_set_irq_clear(&sensor.dev, BGT60CUTR13B_IR_CLEAR_FOR_STARTUP_MSK);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Could not clear the sensor IRQ flags");
        return 1;
    }

    protocol_set_app_info(USE_CASE_NAME, VERSION, VERSION_BUILD);
    protocol_set_sensor_info("BGT60CUTR13B", VERSION, false, 80.0f);

    if (rtt_stream_init(rtt_stream_buffer, sizeof(rtt_stream_buffer)) != 0)
    {
        protocol_print_error("Could not initialize the RTT stream");
        return 1;
    }

    rtt_info.control_block = rtt_stream_get_control_block();

    // the register configuration decides the alignment, so report what the sensor actually does
    uint32_t spi_ctrl = 0;
    if (XENSIV_BGT60CUTRXX_STATUS_OK !=
        xensiv_bgt60cutrxx_get_reg(&sensor.dev, BGT60CUTR13B_REG_SPI_CTRL, &spi_ctrl))
    {
        protocol_print_error("Could not read the sample alignment from the sensor");
        return 1;
    }
    rtt_info.msb_aligned = (spi_ctrl & BGT60CUTR13B_SPI_CTRL_MSB_ALIGN_EN_msk) != 0U;

    protocol_set_rtt_info(&rtt_info);

    protocol_print_info("Radar frame size: %" PRIu32 " samples", (uint32_t)NUM_FRAME_SAMPLES);
    protocol_print_info("FIFO IRQ threshold: %" PRIu32 " samples", (uint32_t)NUM_THRESHOLD_SAMPLES);

    if (NUM_THRESHOLD_SAMPLES != NUM_FRAME_SAMPLES)
    {
        // ADR 0003: one interrupt must deliver one whole radar frame, or the stream is
        // silently misaligned and the host cannot tell
        protocol_print_error("FIFO threshold %" PRIu32 " does not match radar frame size %" PRIu32,
                             (uint32_t)NUM_THRESHOLD_SAMPLES, (uint32_t)NUM_FRAME_SAMPLES);
        return 1;
    }

    result = xensiv_bgt60cutrxx_trigger_sequence0(&sensor.dev);
    if (XENSIV_BGT60CUTRXX_STATUS_OK != result)
    {
        protocol_print_error("Could not start the radar data acquisition sequence");
        return 1;
    }
    protocol_print_info("Radar data acquisition started");

    uint32_t counter = 0;
    uint32_t frames_dropped = 0;

    while (true)
    {
        protocol_handler(counter);

        if (!radar_event_irq)
        {
            continue;
        }
        radar_event_irq = false;

        cyhal_gpio_write(SHIELD_XENSIV_RADAR_LED_GREEN, SHIELD_XENSIV_RADAR_LED_GREEN_ON);

        xensiv_bgt60cutrxx_fifo_events_t event;
        bool frame_valid = false;

        if (XENSIV_BGT60CUTRXX_STATUS_OK != radar_fifo_check_integrity(&sensor.dev, &event, &frame_valid))
        {
            protocol_print_error("Could not read the FIFO event");
            continue;
        }

        if (!frame_valid)
        {
            // defensive: under ADR 0003 the FIFO is fully drained every interrupt, so this
            // branch is not reachable with a matching threshold
            frames_dropped++;
            protocol_print_info("FIFO overflow, radar frame discarded");
            report_frames_dropped(frames_dropped);
            continue;
        }

        if (XENSIV_BGT60CUTRXX_STATUS_OK != radar_fifo_read(&sensor.dev, frame_data_buffer, NUM_FRAME_SAMPLES))
        {
            protocol_print_error("Could not read the radar frame from FIFO");
            continue;
        }

        if (!rtt_stream_write_frame(frame_data_buffer, NUM_FRAME_SAMPLES, counter))
        {
            frames_dropped++;
            report_frames_dropped(frames_dropped);
        }

        cyhal_gpio_write(SHIELD_XENSIV_RADAR_LED_GREEN, SHIELD_XENSIV_RADAR_LED_GREEN_OFF);
        counter++;
    }
}


/* [] END OF FILE */
