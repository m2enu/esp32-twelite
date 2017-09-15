/**
 * Copyright (C) 2017 m2enu
 *
 * @file main/m2x.h
 * @brief AT&T M2X API
 * @author m2enu
 * @date 2017/08/20
 */
#include <stdint.h>
#include <string.h>

#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_request.h"

#define M2X_URL_PFX     "http://api-m2x.att.com/v2/devices/" //!< AT&T M2X URL prefix
#define M2X_URL_SFX     "/update" //!< AT&T M2X URL suffix
#define M2X_KEY_PFX     "X-M2X-KEY: "  //!< AT&T M2X PRIMARY API KEY prefix
#define M2X_ACCEPT      "Accept: */*" //!< AT&T M2X Accept
#define M2X_CONTENT     "application/json" //!< AT&T M2X Content-type
#define M2X_STR_BUF     256 //!< AT&T M2X URL/KEY BUFFER SIZE

#define STATUS_OK       202 //!< HTTP request complete successfully

/** <!-- m2x_request {{{1 -->
 * @brief request to AT&T M2X
 * @param[in] device_id primary device id
 * @param[in] api_key primary api key
 * @param[in] json content
 * @param[in] retry maximum retry number
 * @return result of HTTP request
 * @retval 202: OK
 */
int32_t m2x_request(char *device_id, char *api_key, char *json, int32_t retry);

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
