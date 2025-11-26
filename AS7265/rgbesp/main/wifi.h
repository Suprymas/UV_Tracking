#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void wifi_init_sta(void);
extern EventGroupHandle_t wifi_event_group;
extern const int WIFI_CONNECTED_BIT;
