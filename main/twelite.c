/**
 * Copyright (C) 2017 m2enu
 *
 * @file main/twelite.c
 * @brief MONO WIRELESS TWE-LITE app_tag packet parser
 * @author m2enu
 * @date 2017/08/20
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "twelite.h"

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
int8_t substring(char *dst, const char *src, int32_t pos, int32_t len)
{
    if ((pos < 0) || (len < 0) || (len > strlen(src))) {
        return -1;
    }
    for (src += pos; (*src != '\0') && (len > 0); len--) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return 0;
}

/** <!-- hex2dec {{{1 -->
 * @brief convert hexadecimal-string to unsigned-decimal-integer function
 * @param[in] src source hexadecimal-string
 * @param[in] base base number (allow only 10 or 16)
 * @return converted value
 */
uint32_t hex2dec(char *src, int32_t base)
{
    if ((base != 10) && (base != 16)) {
        return 0;
    }
    uint64_t tmp = strtoul(src, NULL, base);
    return (uint32_t)tmp;
}

/** <!-- parse {{{1 -->
 * @brief macro of substring -> hex2dec
 * @param[in] src received packet of TWE-LITE
 * @param[in] base base number (allow only 10 or 16)
 * @param[in] pos positin of source string
 * @param[in] len length of substring
 * @return parsed parameter
 */
uint32_t parse(const char *src, int32_t base, int32_t pos, int32_t len)
{
    char tmp[TWELITE_PACKET_LENGTH_MAX] = {0};
    if (substring(tmp, src, pos, len)) {
        return 0;
    }
    return hex2dec(tmp, base);
}

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
int8_t twelite_parse_packet(twelite_packet_t *pkt, char *data, int32_t len)
{
    if (len < TWELITE_PACKET_LENGTH_MIN) {
        return -1;
    }
    // TODO: compare checksum
    pkt->ok                         = 1;
    pkt->sid_router                 = parse(data, 16,  1, 8);
    pkt->lqi                        = parse(data, 16,  9, 2);
    pkt->next_number                = parse(data, 16, 11, 4);
    pkt->sid_enddevice              = parse(data, 16, 15, 8);
    pkt->id_enddevice               = parse(data, 16, 23, 2);
    pkt->id_sensor                  = parse(data, 16, 25, 2);
    pkt->mvolt_vdd                  = parse(data, 16, 27, 2);
    pkt->mvolt_adc1                 = parse(data, 16, 29, 4);
    pkt->mvolt_adc2                 = parse(data, 16, 33, 4);
    pkt->pkt_bme280.id_sensor       = parse(data, 16, 37, 4);
    pkt->pkt_bme280.i_temperature   = parse(data, 16, 41, 4);
    pkt->pkt_bme280.i_humidity      = parse(data, 16, 45, 4);
    pkt->pkt_bme280.i_pressure      = parse(data, 16, 49, 8);
    pkt->checksum                   = parse(data, 16, 57, 2);

    pkt->mvolt_vdd      = twelite_calc_supply((uint8_t)(pkt->mvolt_vdd & 0xff));
    pkt->checksum_calc  = twelite_calc_checksum(data, 0, len);

    twelite_parse_packet_bme280(pkt);

    return 0;
}

/** <!-- twelite_parse_packet_bme280 {{{1 -->
 * @brief packet parser for TWE-LITE app_tag BME280 sensor data
 * @param[out] pkt TWE-LITE packet
 * @return result of parse
 * @retval Zero: Success
 * @retval +ve_value: Warning
 * @retval -ve_value: Error
 */
int8_t twelite_parse_packet_bme280(twelite_packet_t *pkt)
{
    pkt->pkt_bme280.temperature = pkt->pkt_bme280.i_temperature / 100.0;
    pkt->pkt_bme280.humidity    = pkt->pkt_bme280.i_humidity    / 100.0;
    pkt->pkt_bme280.pressure    = pkt->pkt_bme280.i_pressure    /   1.0;
    return 0;
}

/** <!-- twelite_calc_supply {{{1 -->
 * @brief calculate TWE-LITE end device power supply voltage
 * @param[in] src packet of power supply voltage (1byte data)
 * @return power supply voltage in mV
 */
int16_t twelite_calc_supply(uint8_t src)
{
    if (src <= 170) {
        return 1950 + src * 5; // 1.95 - 2.80 V
    }
    return 2800 + (src - 170) * 10; // 2.81 - 3.65 V
}

/** <!-- twelite_calc_checksum {{{1 -->
 * @brief calculate TWE-LITE app_tag packet checksum
 * @param[in] data received TWE-LITE app_tag string (ascii)
 * @param[in] pos start position of data
 * @param[in] len length of data
 * @return calculated checksum
 */
uint8_t twelite_calc_checksum(char *data, int32_t pos, int32_t len)
{
    // TODO: calculation of checksum is unknown
    int32_t i;
    uint8_t ret = 0;
    for (i = pos; i < len; i++) {
        ret = ret ^ data[i];
    }
    return ret;
}

/** <!-- debug_twelite_print_packet {{{1 -->
 * @brief debug function of printing packet parameters
 * @param nothing
 * @return nothing
 */
void debug_twelite_print_packet(twelite_packet_t *pkt)
{
    printf("\n");
    printf("parse           : %8d  \n", pkt->ok                         );
    printf("sid_router      : %08x \n", pkt->sid_router                 );
    printf("lqi             : %08x \n", pkt->lqi                        );
    printf("next_number     : %08x \n", pkt->next_number                );
    printf("sid_enddevice   : %08x \n", pkt->sid_enddevice              );
    printf("id_enddevice    : %08x \n", pkt->id_enddevice               );
    printf("id_sensor       : %08x \n", pkt->id_sensor                  );
    printf("mvolt_vdd       : %8d  \n", pkt->mvolt_vdd                  );
    printf("mvolt_adc1      : %8d  \n", pkt->mvolt_adc1                 );
    printf("mvolt_adc2      : %8d  \n", pkt->mvolt_adc2                 );
    printf("BME280 id       : %08x \n", pkt->pkt_bme280.id_sensor       );
    printf("BME280 temp(int): %8d  \n", pkt->pkt_bme280.i_temperature   );
    printf("BME280 humi(int): %8d  \n", pkt->pkt_bme280.i_humidity      );
    printf("BME280 pres(int): %8d  \n", pkt->pkt_bme280.i_pressure      );
    printf("BME280 temp(flt): %8.2f\n", pkt->pkt_bme280.temperature     );
    printf("BME280 humi(flt): %8.2f\n", pkt->pkt_bme280.humidity        );
    printf("BME280 pres(flt): %8.2f\n", pkt->pkt_bme280.pressure        );
    printf("checksum        : %08x \n", pkt->checksum                   );
    printf("checksum_calc   : %08x \n", pkt->checksum_calc              );
}

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
