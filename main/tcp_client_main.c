#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"

// Configuración de red y hardware
#define WIFI_NETWORK  "PUCESE_WIFI"  // Nombre de la red WiFi
#define WIFI_PASSWORD "Euskadi1981"     // Contraseña de la red WiFi
#define SERVER_ADDRESS "10.10.65.46"   // Dirección IP del servidor
#define SERVER_PORT 12345               // Puerto del servidor
#define LED_PIN GPIO_NUM_2              // GPIO donde está conectado el LED

// Etiqueta para los logs
static const char *LOG_TAG = "ESP32_TCP_CLIENT";

// Configura la conexión WiFi en modo estación
void setup_wifi() {
    esp_netif_init();  // Inicializa la red
    esp_event_loop_create_default();  // Crea el bucle de eventos predeterminado
    esp_netif_create_default_wifi_sta();  // Configura el modo estación
    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_cfg);

    // Configuración de la red WiFi
    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = WIFI_NETWORK,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);  // Configura el modo estación
    esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);  // Aplica la configuración
    esp_wifi_start();  // Inicia el WiFi
    esp_wifi_connect();  // Conecta a la red
}

// Cambia el estado del LED y envía el estado al servidor
void toggle_led_and_notify(int socket_fd) {
    static int led_state = 0;  // Estado actual del LED (0 = apagado, 1 = encendido)
    led_state = !led_state;  // Alterna el estado del LED
    gpio_set_level(LED_PIN, led_state);  // Actualiza el GPIO del LED

    // Prepara el mensaje para enviar al servidor
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "LED está: %s", led_state ? "ACTIVO" : "INACTIVO");

    // Envía el mensaje al servidor
    if (send(socket_fd, buffer, strlen(buffer), 0) < 0) {
        ESP_LOGE(LOG_TAG, "Error al enviar el mensaje al servidor");
    } else {
        ESP_LOGI(LOG_TAG, "Mensaje enviado: %s", buffer);
    }
}

// Tarea principal del cliente TCP
void tcp_client_task(void *params) {
    char receive_buffer[128];  // Buffer para recibir datos
    struct sockaddr_in server_info;  // Información del servidor

    while (1) {
        // Crea un socket
        int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            ESP_LOGE(LOG_TAG, "No se pudo crear el socket");
            vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de reintentar
            continue;
        }

        // Configura la dirección del servidor
        server_info.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
        server_info.sin_family = AF_INET;
        server_info.sin_port = htons(SERVER_PORT);

        ESP_LOGI(LOG_TAG, "Intentando conectar al servidor %s:%d...", SERVER_ADDRESS, SERVER_PORT);

        // Intenta conectar al servidor
        while (connect(socket_fd, (struct sockaddr *)&server_info, sizeof(server_info)) < 0) {
            ESP_LOGE(LOG_TAG, "No se pudo conectar al servidor. Reintentando en 3 segundos...");
            vTaskDelay(pdMS_TO_TICKS(3000));
        }

        ESP_LOGI(LOG_TAG, "Conexión establecida con el servidor");

        // Bucle principal de comunicación
        while (1) {
            toggle_led_and_notify(socket_fd);  // Cambia el estado del LED y envía el estado
            int len = recv(socket_fd, receive_buffer, sizeof(receive_buffer) - 1, 0);  // Recibe datos
            if (len > 0) {
                receive_buffer[len] = '\0';  // Termina la cadena recibida
                ESP_LOGI(LOG_TAG, "Respuesta del servidor: %s", receive_buffer);
            } else {
                ESP_LOGE(LOG_TAG, "El servidor cerró la conexión. Cerrando socket...");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de enviar el siguiente mensaje
        }

        close(socket_fd);  // Cierra el socket
        ESP_LOGI(LOG_TAG, "Reintentando conexión en 5 segundos...");
        vTaskDelay(pdMS_TO_TICKS(5000));  // Espera antes de reintentar
    }
}

// Punto de entrada principal de la aplicación
void app_main() {
    ESP_LOGI(LOG_TAG, "Iniciando cliente TCP en ESP32...");
    nvs_flash_init();  // Inicializa el almacenamiento no volátil
    setup_wifi();  // Configura la conexión WiFi
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);  // Configura el GPIO del LED como salida
    xTaskCreate(&tcp_client_task, "tcp_client_task", 4096, NULL, 5, NULL);  // Crea la tarea del cliente TCP
}