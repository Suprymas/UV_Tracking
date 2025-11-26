idf_component_register(
    SRCS "websocket.c" "wifi.c" "as7265x.c" "i2c_driver.c" "main.c"
    INCLUDE_DIRS "."
    REQUIRES 
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        esp_websocket_client
        driver
        json
)