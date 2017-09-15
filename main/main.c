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
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "soc/uart_struct.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "twelite.h"
#include "m2x.h"

// global members {{{1
static const char *TAG = "main"; //!< ESP_LOGx tag
static const char *TAG_WIFI = "wifi"; //!< ESP_LOGx tag for WiFi
static EventGroupHandle_t wifi_event_group; //!< FreeRTOS event group to signal when we are connected & ready to make a request
/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;

// defines {{{1
#define UART_TXD_PIN    (4) //!< GPIO number of UART TXD
#define UART_RXD_PIN    (5) //!< GPIO number of UART RXD
#define UART_RTS_PIN    (18) //!< GPIO number of UART RTS
#define UART_CTS_PIN    (19) //!< GPIO number of UART CTS
#define UART_BAUDRATE   115200 //!< UART baudrate [bps]
#define UART_NUM        UART_NUM_1 //!< port number of UART
#define UART_TIMEOUT    20 / portTICK_RATE_MS //!< UART TIMEOUT [ms]
#define UART_BUF_SIZE   (1024) //!< UART receive buffer size

#define WIFI_SSID       CONFIG_WIFI_SSID //!< WiFi SSID
#define WIFI_PASS       CONFIG_WIFI_PASSWORD //!< WiFi PASSWORD

#define M2X_POST_PIN    (19) //!< GPIO number for switching M2X post
#define M2X_ID          CONFIG_M2X_ID //!< AT&T M2X PRIMARY DEIVCE ID
#define M2X_KEY         CONFIG_M2X_KEY //!< AT&T M2X PRIMARY API KEY
#define M2X_RETRY       10 //!< M2X POST retry number

/** <!-- event_handler {{{1 -->
 * @brief event handler
 * @param[in] ctx
 * @param[in] event
 * @return error code
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

/** <!-- wifi_init {{{1 -->
 * @brief WiFi Initialisation
 * @param[in] ssid WiFi SSID
 * @param[in] pass WiFi PASSWORD
 * @return nothing
 */
static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        }
    };
    ESP_LOGI(TAG_WIFI,
             "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/** <!-- wifi_connect {{{1 -->
 * @brief connect to access point
 * @param nothing
 * @return nothing
 */
static void wifi_connect(void)
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    ESP_LOGI(TAG_WIFI, "Connected to AP, freemem=%d", esp_get_free_heap_size());
}

/** <!-- create_json {{{1 -->
 * @brief create json message from TWE-LITE packet
 * @param[out] dst json string
 * @param[in] pkt TWE-LITE app_tag packet structure
 * @return nothing
 */
void create_json(char *dst, twelite_packet_t *pkt)
{
    const char *fmt = (
        "{ \"values\": {"
            "\"temperature\": %6.2f" ", "
            "\"pressure\": %9.2f" ", "
            "\"humidity\": %6.2f" ", "
            "\"vdd\": %6.3f"
        "} }"
    );
    sprintf(dst, fmt,
            pkt->pkt_bme280.temperature,
            pkt->pkt_bme280.pressure,
            pkt->pkt_bme280.humidity,
            (pkt->mvolt_vdd / 1000.0));
}

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
    char json[256];
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
        // post to M2X
        create_json(json, &pkt);
        ESP_LOGI(TAG, "%s", json);
        if (gpio_get_level(M2X_POST_PIN) == 0) {
            ESP_LOGI(TAG, "M2X POST disable -> continue ...");
            continue;
        }
        m2x_request(M2X_ID, M2X_KEY, json, M2X_RETRY);
    }
}

/** <!-- gpio_init {{{1 -->
 * @brief GPIO initialisation
 * @param nothing
 * @return nothing
 */
void gpio_init(void)
{
    gpio_set_direction(M2X_POST_PIN, GPIO_MODE_INPUT);
}

/** <!-- app_main {{{1 -->
 * @brief main function
 * @return nothing
 */
void app_main(void)
{
    // initialise peripherals
    nvs_flash_init();
    gpio_init();
    uart_init();
    wifi_init();
    // connect to access point
    wifi_connect();
    // create tasks
    xTaskCreate(uart_task, "uart_echo_task", 4096, NULL, 10, NULL);
}

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
