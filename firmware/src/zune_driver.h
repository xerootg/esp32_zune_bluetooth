#pragma once
#include "stdint.h"
#include "esp_avrc_api.h"

void setupZune(int tx_pin, int rx_pin, int _auth_pin);
void handle_avrcp(esp_avrc_tg_cb_param_t::avrc_tg_psth_cmd_param);
void zune_message_receiver(void* pvParameters);