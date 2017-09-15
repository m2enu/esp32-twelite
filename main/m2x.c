/**
 * Copyright (C) 2017 m2enu
 *
 * @file main/m2x.c
 * @brief AT&T M2X API
 * @author m2enu
 * @date 2017/08/20
 */
#include "m2x.h"

const char *TAG_M2X = "m2x"; //!< ESP_LOGx TAG for M2X

/** <!-- m2x_request {{{1 -->
 * @brief request to AT&T M2X
 * @param[in] device_id primary device id
 * @param[in] api_key primary api key
 * @param[in] json content
 * @return result of HTTP request
 * @retval 202: OK
 */
int32_t m2x_request(char *device_id, char *api_key, char *json, int32_t retry)
{
    request_t *req;
    int32_t status = 0;
    char m2x_url[M2X_STR_BUF];
    char m2x_key[M2X_STR_BUF];
    sprintf(m2x_url, "%s%s%s", M2X_URL_PFX, device_id, M2X_URL_SFX);
    sprintf(m2x_key, "%s%s", M2X_KEY_PFX, api_key);

    int32_t n = 0;
    for (n = 0; n < retry; n++) {
        req = req_new(m2x_url);
        req_setopt(req, REQ_SET_HEADER, M2X_ACCEPT);
        req_setopt(req, REQ_SET_HEADER, m2x_key);
        req_setopt(req, REQ_SET_POSTFIELDS, M2X_CONTENT);
        req_setopt(req, REQ_SET_DATAFIELDS, json);
        status = req_perform(req);
        req_clean(req);
        if (status == STATUS_OK) {
            ESP_LOGI(TAG_M2X, "Finish request, status=%d", status);
            break;
        } else {
            ESP_LOGE(TAG_M2X, "Failed request, status=%d (%d/%d)",
                     status, (n+1), retry);
        }
    }

    return status;
}

// end of file {{{1
// vim:ft=c:et:nowrap:fdm=marker
