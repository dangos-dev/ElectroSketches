/*
  Título: Receptor ESP32 - Recepción de Datos vía ESP-NOW
  Descripción: Dispositivo receptor inalámbrico para datos estructurados.
               Funcionalidades:
               - Recepción asíncrona usando protocolo ESP-NOW
               - Procesamiento básico de datos recibidos
  Autor: Jabes (Dango)
  Fecha: 10 de marzo del 2025
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Estructura debe coincidir con el emisor
typedef struct struct_message {
  char message[32];
  int value;
  float sensor_data;
  bool status;
} struct_message;

// Instancia para almacenar datos recibidos
struct_message receivedData;


void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  
  Serial.print("Datos recibidos: ");
  Serial.print(receivedData.message);
  Serial.print(" | ");
  Serial.print(receivedData.value);
  Serial.print(" | ");
  Serial.print(receivedData.sensor_data);
  Serial.print(" | ");
  Serial.println(receivedData.status ? "Activo" : "Inactivo");
  
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
}


void setup() {
  uint8_t baseMac[6];
  
  Serial.begin(115200);
  pinMode(2, OUTPUT);

  // Mostrar MAC address
  WiFi.mode(WIFI_STA);
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.print("MAC Receptor: ");
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                 baseMac[0], baseMac[1], baseMac[2],
                 baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Error leyendo MAC");
  }

  // Inicializar ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inicializando ESP-NOW");
    return;
  }

  // Registrar callback de recepción
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}


void loop() {
  // Mantener vacío por uso de callback
}