/**
 * Copyright (C) 2017 m2enu
 *
 * @file main/twelite.h
 * @brief MONO WIRELESS TWE-LITE app_tag packet parser
 * @author m2enu
 * @date 2017/08/20
 */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define TWELITE_PACKET_LENGTH_MIN   37 //!< TWE-LITE app_tag packet length min.
#define TWELITE_PACKET_LENGTH_MAX   61 //!< TWE-LITE app_tag packet length max.

/** <!-- twelite_packet_bme280_t {{{1 -->
 * @brief TWE-LITE app_tag packet structure for BME280 sensor data
 */
typedef struct twelite_packet_bme280_t_tag {
    uint16_t id_sensor; //!< sensor id
    uint16_t i_temperature; //!< temperature in integer [x100 degC]
    uint16_t i_humidity; //!< humidity in integer [x100 %]
    uint32_t i_pressure; //!< pressure in integer [Pa]
    float temperature; //!< temperature [degC]
    float humidity; //!< humidity [%]
    float pressure; //!< pressure [Pa]
} twelite_packet_bme280_t;

/** <!-- twelite_packet_t {{{1 -->
 * @brief TWE-LITE app_tag packet structure
 */
typedef struct twelite_packet_t_tag {
    int8_t ok; //!< 0:parse failed, 1:parse successed
    uint32_t sid_router; //!< SID of router
    uint8_t lqi; //!< LQI
    uint16_t next_number; //!< Next number
    uint32_t sid_enddevice; //!< SID of end device
    uint8_t id_enddevice; //!< ID of end device
    uint8_t id_sensor; //!< ID of sensor
    uint16_t mvolt_vdd; //!< power supply voltage [mV]
    uint16_t mvolt_adc1; //!< ADC1 voltage [mV]
    uint16_t mvolt_adc2; //!< ADC2 voltage [mV]
    uint8_t checksum; //!< received checksum
    uint8_t checksum_calc; //!< calculated checksum

    twelite_packet_bme280_t pkt_bme280; //!< packet for BME280 sensor data
} twelite_packet_t;

/** <!-- substring {{{1 -->
 * @brief substring function
 * @param[out] dst destination string
 * @param[in] src source string
 * @param[in] pos position of source string
 * @param[in] len length of substring
 * @return result of substring
 * @retval Zero: Success
 * @retval +ve_value: Warning
 * @retval -ve_value: Error
 */
int8_t substring(char *dst, const char *src, int32_t pos, int32_t len);

/** <!-- hex2dec {{{1 -->
 * @brief convert hexadecimal-string to unsigned-decimal-integer function
 * @param[in] src source hexadecimal-string
 * @param[in] base base number (allow only 10 or 16)
 * @return converted value
 */
uint32_t hex2dec(char *src, int32_t base);

/** <!-- parse {{{1 -->
 * @brief macro of substring -> hex2dec
 * @param[in] src received packet of TWE-LITE
 * @param[in] base base number (allow only 10 or 16)
 * @param[in] pos positin of source string
 * @param[in] len length of substring
 * @return parsed parameter
 */
uint32_t parse(const char *src, int32_t base, int32_t pos, int32_t len);

/** <!-- twelite_parse_packet {{{1 -->
 * @brief packet parser for TWE-LITE app_tag
 * @param[out] pkt TWE-LITE packet
 * @param[in] data received TWE-LITE app_tag string (ascii)
 * @param[in] len length of data
 * @return result of parse
 * @retval Zero: Success
 * @retval +ve_value: Warning
 * @retval -ve_value: Error
 */
int8_t twelite_parse_packet(twelite_packet_t *pkt, char *data, int32_t len);

/** <!-- twelite_parse_packet_bme280 {{{1 -->
 * @brief packet parser for TWE-LITE app_tag BME280 sensor data
 * @param[out] pkt TWE-LITE packet
 * @return result of parse
 * @retval Zero: Success
 * @retval +ve_value: Warning
 * @retval -ve_value: Error
 */
int8_t twelite_parse_packet_bme280(twelite_packet_t *pkt);

/** <!-- twelite_calc_supply {{{1 -->
 * @brief calculate TWE-LITE end device power supply voltage
 * @param[in] src packet of power supply voltage (1byte data)
 * @return power supply voltage in mV
 */
int16_t twelite_calc_supply(uint8_t src);

/** <!-- twelite_calc_checksum {{{1 -->
 * @brief calculate TWE-LITE app_tag packet checksum
 * @param[in] data received TWE-LITE app_tag string (ascii)
 * @param[in] pos start position of data
 * @param[in] len length of data
 * @return calculated checksum
 */
uint8_t twelite_calc_checksum(char *data, int32_t pos, int32_t len);

/** <!-- debug_twelite_print_packet {{{1 -->
 * @brief debug function of printing packet parameters
 * @param nothing
 * @return nothing
 */
void debug_twelite_print_packet(twelite_packet_t *pkt);

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
