/*
  Título: Orquestación de Tareas en ESP32 Dual-Core
  Descripción: Ejemplo de multitarea en ESP32 para controlar dos LEDs en núcleos separados.
  Autor: Jabes (Dango)
  Fecha: 15 de febrero 2025
*/

TaskHandle_t taskCore0Handle;
TaskHandle_t taskCore1Handle;

// Variables globales para los tiempos de delay
int delayTimeCore0 = 300; // * 10e-3 segundos
int delayTimeCore1 = 1000; // * 10e-3 segundos

// Variables globales para los leds
int pinCore0 = 13;
int pinCore1 = 12;

void setup() {
  Serial.begin(115200);

  // Crea las tareas con stack depth de 1000 bytes
  xTaskCreatePinnedToCore(loopCore0, "taskCore0", 1000, NULL, 1, &taskCore0Handle, 0); // Núcleo 0
  xTaskCreatePinnedToCore(loopCore1, "taskCore1", 1000, NULL, 1, &taskCore1Handle, 1); // Núcleo 1

  pinMode(pinCore0, OUTPUT);
  pinMode(pinCore1, OUTPUT);
}

// !!!
void loop() {
  vTaskDelete(NULL); // Eliminar la tarea loop (no necesaria)
}

void loopCore0(void *parameter) {
  while (1) {
    Serial.println("Ejecutando en core: " + String(xPortGetCoreID()));

    digitalWrite(pinCore0, HIGH);
    delay(delayTimeCore0);
    digitalWrite(pinCore0, LOW);
    delay(delayTimeCore0);
  }
}

void loopCore1(void *parameter) {
  while (1) {
    Serial.println("Ejecutando en core: " + String(xPortGetCoreID()));

    digitalWrite(pinCore1, HIGH);
    delay(delayTimeCore1);
    digitalWrite(pinCore1, LOW);
    delay(delayTimeCore1);
  }
}