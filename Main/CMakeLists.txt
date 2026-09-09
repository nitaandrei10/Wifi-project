idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS ""
    REQUIRES 
        freertos
        esp_wifi
        esp_event
        nvs_flash
        log
        esp_system
        esp_http_client
        esp-tls
        json
)