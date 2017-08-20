/**
 * Copyright (C) 2017 m2enu
 *
 * @file main/main.c
 * @brief Bosch Sensortec BME280 Pressure sensor logger via TWE-LITE
 * @author m2enu
 * @date 2017/08/19
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

#include "twelite.h"

static const char *TAG = "main"; //!< ESP_LOGx tag

// defines {{{1
#define UART_TXD_PIN    (4) //!< GPIO number of UART TXD
#define UART_RXD_PIN    (5) //!< GPIO number of UART RXD
#define UART_RTS_PIN    (18) //!< GPIO number of UART RTS
#define UART_CTS_PIN    (19) //!< GPIO number of UART CTS
#define UART_BAUDRATE   115200 //!< UART baudrate [bps]
#define UART_NUM        UART_NUM_1 //!< port number of UART
#define UART_TIMEOUT    20 / portTICK_RATE_MS //!< UART TIMEOUT [ms]
#define UART_BUF_SIZE   (1024) //!< UART receive buffer size

/** <!-- uart_init {{{1 -->
 * @brief UART peripheral initialisation
 * @return nothing
 */
void uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM,
                 UART_TXD_PIN, UART_RXD_PIN, UART_RTS_PIN, UART_CTS_PIN);
    uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}

/** <!-- uart_task {{{1 -->
 * @brief UART receive and display task
 * @return nothing
 */
static void uart_task(void *args)
{
    twelite_packet_t pkt;
    uint8_t* data = (uint8_t*) malloc(UART_BUF_SIZE);
    while(1) {
        // Read data from UART
        int32_t len = uart_read_bytes(UART_NUM, data,
                                      UART_BUF_SIZE, UART_TIMEOUT);
        if (len <= 0) continue;

        // parse twe-lite packet
        int8_t err = twelite_parse_packet(&pkt, (char*)data, len);
        if (err < 0) {
            ESP_LOGE(TAG, "TWE-LITE packet parse failed: %d", err);
            continue;
        }
        // print data
        debug_twelite_print_packet(&pkt);
    }
}

/** <!-- app_main {{{1 -->
 * @brief main function
 * @return nothing
 */
void app_main(void)
{
    // initialise peripherals
    uart_init();
    // create tasks
    xTaskCreate(uart_task, "uart_echo_task", 4096, NULL, 10, NULL);
}

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
