/*
  Título: Emisor ESP32 - Transmisión de Datos vía ESP-NOW
  Descripción: Dispositivo emisor inalámbrico que genera y transmite datos estructurados.
               Funcionalidades:
               - Generación periódica de datos simulados (valor entero, flotante y booleano)
               - Buffer de datos mediante colas para manejo de paquetes
               - Configuración MAC específica para comunicación punto a punto
  Autor: Jabes (Dango)
  Fecha: 10 de marzo del 2025
  Destino: Receptor en MAC E8:6B:EA:D4:27:D0
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>


#define QUEUE_SIZE 5
#define MIN_INTERVAL 500
#define MAX_INTERVAL 3000

// MAC de la placa receptora
uint8_t broadcastAddress[] = { 0xE8, 0x6B, 0xEA, 0xD4, 0x27, 0xD0 };

typedef struct struct_message {
  char message[32];
  int value;
  float sensor_data;
  bool status;
} struct_message;

QueueHandle_t dataQueue;
esp_now_peer_info_t peerInfo;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Falló");
  digitalWrite(2, HIGH);
  delay(100);
  digitalWrite(2, LOW);
}


// Tarea en Core 0: Manejo de comunicación
void senderTask(void *pvParameters) {
  while (1) {
    struct_message receivedData;
    if (xQueueReceive(dataQueue, &receivedData, portMAX_DELAY)) {
      esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&receivedData, sizeof(receivedData));

      if (result != ESP_OK) {
        Serial.println("Error en envío desde tarea");
      }
    }
  }
}


// Tarea en Core 1: Procesamiento de datos
void dataTask(void *pvParameters) {
  while (1) {
    // Generar intervalo aleatorio
    uint32_t delayTime = (esp_random() % (MAX_INTERVAL - MIN_INTERVAL)) + MIN_INTERVAL;
    vTaskDelay(delayTime / portTICK_PERIOD_MS);

    // Crear nuevos datos
    struct_message newData;
    snprintf(newData.message, sizeof(newData.message), "Timestamp: %lu", millis());

    newData.value = esp_random() % 1000;
    newData.sensor_data = (float)(esp_random() % 1000) / 100.0;
    newData.status = (esp_random() % 2) == 0;

    // Enviar a la cola
    if (xQueueSend(dataQueue, &newData, 0) != pdTRUE) {
      Serial.println("Cola llena, descartando datos");
    }
  }
}


void setup() {
  uint8_t baseMac[6];

  Serial.begin(115200);
  pinMode(2, OUTPUT);

  // Mostrar MAC address
  WiFi.mode(WIFI_STA);
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Configurar peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Error configurando peer");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  // Cola para comunicación entre núcleos
  dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(struct_message));

  // Crear tareas en núcleos diferentes
  xTaskCreatePinnedToCore(senderTask, "SenderTask", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(dataTask,   "dataTask",   10000, NULL, 2, NULL, 1);
}


// !!!
void loop() {
  vTaskDelete(NULL);  // Eliminar la tarea loop (no necesaria)
}