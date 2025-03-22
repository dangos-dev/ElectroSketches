/*
  -------------------------------------------------------------------------------------------------
  |                                                                                               |
  |   Título:      Sistema de Reserva de Asientos de Avión con Matriz LED y ESP32                 |
  |                                                                                               |
  |   Descripción: Implementación del ejercicio propuesto por el Prof. Jean Charly Ph. Joseph S.  |
  |                del ITLA.  Este programa simula un sistema de reserva de asientos para un avión|
  |                utilizando un ESP32, una matriz LED 8x8, un LED RGB y un botón pulsador.  El   |
  |                sistema gestiona la asignación de asientos en tres clases (Primera Clase,      |
  |                Segunda Clase y Tercera Clase) según el costo del billete y la edad del        |
  |                pasajero.  La ocupación de los asientos se visualiza en la matriz LED y la     |
  |                clase del último pasajero asignado se indica con el LED RGB. Se implementa     |
  |                multitarea utilizando FreeRTOS en el ESP32, con una tarea dedicada a la        |
  |                gestión de la matriz LED y otra para la interacción con el usuario (botón y    |
  |                lógica de asignación).                                                         |
  |                                                                                               |
  |   Autor:       Jabes Rivas (Dango)                                                            |
  |                                                                                               |
  |   Fecha:       21 de marzo de 2025 (Última modificación)                                      |
  -------------------------------------------------------------------------------------------------
*/

#include <ControlDM11A88.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Pines para la matriz LED
// ├── DI (Data IN) -> GPIO23 (VSPI MOSI)
// ├── CLK = Clock  -> GPIO18 (VSPI CLK)
// └── LAT = Latch  -> GPIO5  (VSPI CS0)

ControlDM11A88 lc(23, 18, 5, 1);

// Pin del botón GPIO15 (HSPI CS0)
const int buttonPin = 15;

// Pines del LED RGB (Common Anode)
#define RGB_LED_RED_PIN 4     // Pin para el color rojo
#define RGB_LED_GREEN_PIN 16  // Pin para el color verde
#define RGB_LED_BLUE_PIN 17   // Pin para el color azul


// Estructura para representar un asiento
// Indica si está ocupado y quién lo ocupa
struct Asiento {
  bool ocupado;
  String nombrePasajero;
};

// La matriz de asientos. 3 clases, 4 filas, 3 asientos por fila
Asiento clases[3][4][3];

SemaphoreHandle_t mutex;
uint8_t displayBuffer[8] = { 0 };  // 8 filas, un byte por fila.

// Almacena los datos del último pasajero
String nombreUltimoPasajero = "";
int claseUltimoPasajero = -1;


// Tarea que actualiza la matriz LED
void actualizarMatrizLEDTask(void *pvParameters) {
  for (;;) {  // Bucle infinito

    // Pasa el contenido del buffer a la matriz, fila por fila.
    for (int filaLED = 0; filaLED < 8; filaLED++) {
      lc.setRow(0, filaLED, displayBuffer[filaLED]);
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}


// Tarea para el botón y agregar pasajeros
void agregarPasajeroTask(void *pvParameters) {
  static unsigned long ultimoTiempo = 0;  // Control de debounce para boton

  for (;;) {
    bool estadoBoton = digitalRead(buttonPin);

    if (estadoBoton == LOW && millis() - ultimoTiempo > 200) {
      ultimoTiempo = millis();

      // Intenta tomar el mutex.  Espera si otra tarea lo está usando
      if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
        agregarPasajero();         // Llama a la función para añadir un pasajero
        imprimirEstadoAsientos();  // Muestra el estado en el monitor serial
        updateDisplayBuffer();     // Refleja los cambios en el buffer de la pantalla
        updateRGBLED();            // Actualiza el LED RGB con la clase del último pasajero
        xSemaphoreGive(mutex);     // Libera el mutex
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));  // Espera 50ms para debounce del boton
  }
}

// Función para actualizar el LED RGB basado en la clase del ultimo pasajero
void updateRGBLED() {
  // Apaga todos los colores primero
  digitalWrite(RGB_LED_RED_PIN, HIGH);
  digitalWrite(RGB_LED_GREEN_PIN, HIGH);
  digitalWrite(RGB_LED_BLUE_PIN, HIGH);


  if (claseUltimoPasajero == 0) {
    digitalWrite(RGB_LED_RED_PIN, LOW);  // Enciende rojo
  } else if (claseUltimoPasajero == 1) {
    digitalWrite(RGB_LED_GREEN_PIN, LOW);  // Enciende verde
  } else if (claseUltimoPasajero == 2) {
    digitalWrite(RGB_LED_BLUE_PIN, LOW);  // Enciende azul
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(RGB_LED_RED_PIN, OUTPUT);
  pinMode(RGB_LED_GREEN_PIN, OUTPUT);
  pinMode(RGB_LED_BLUE_PIN, OUTPUT);

  digitalWrite(RGB_LED_RED_PIN, HIGH);
  digitalWrite(RGB_LED_GREEN_PIN, HIGH);
  digitalWrite(RGB_LED_BLUE_PIN, HIGH);


  // Pone todos los asientos como vacíos al principio
  for (int c = 0; c < 3; c++) {      // Recorre las clases
    for (int f = 0; f < 4; f++) {    // Recorre las filas
      for (int a = 0; a < 3; a++) {  // Recorre los asientos
        clases[c][f][a].ocupado = false;
        clases[c][f][a].nombrePasajero = "*";
      }
    }
  }

  lc.clearDisplay();  // Apaga todos los LEDs de la matriz al inicio
  mutex = xSemaphoreCreateMutex();

  if (mutex == NULL) {
    Serial.println("Error al crear el mutex");
    while (1)
      ;  // Se queda en un bucle infinito si dio error
  }

  xTaskCreatePinnedToCore(actualizarMatrizLEDTask, "MatrizLED", 2048, NULL, 2, NULL, 0);    // Núcleo 0
  xTaskCreatePinnedToCore(agregarPasajeroTask, "AgregarPasajero", 4096, NULL, 1, NULL, 1);  // Núcleo 1
}

// !!!
void loop() {
  vTaskDelete(NULL);  // Eliminar la tarea loop (no necesaria)
}

// Función para asignar un pasajero a un asiento (si hay alguno libre)
void agregarPasajero() {
  String nombre = "Pasajero" + String(random(1000));
  int edad = random(10, 80);
  int costo = random(0, 1500);

  int clase = determinarClase(costo);  // Decide la clase según el costo
  if (clase == -1) return;

  bool extremo = determinarTipoAsiento(clase, edad);  // Decide si ventana/pasillo
  int fila, asiento;

  // Intenta encontrar un asiento
  if (!asignarAsiento(clase, extremo, fila, asiento)) {
    Serial.println("Clase " + String(clase) + " llena.  Buscando otro vuelo.");
    return;  // Si no hay asiento, sale.
  }

  // Si encontró asiento, lo marca como ocupado y guarda el nombre
  clases[clase][fila][asiento].ocupado = true;
  clases[clase][fila][asiento].nombrePasajero = nombre;

  // Almacena la información del último pasajero
  nombreUltimoPasajero = nombre;
  claseUltimoPasajero = clase;

  // Muestra información del pasajero en el monitor serial
  Serial.println("Nuevo pasajero:");
  Serial.println("Nombre: " + nombre);
  Serial.println("Edad: " + String(edad));
  Serial.println("Costo: $" + String(costo));
  Serial.println("Clase: " + String(clase));
  Serial.println("Asiento: Fila " + String(fila) + ", Asiento " + String(asiento));
  Serial.println("-----------------------");
}

// Decide la clase del pasajero según el costo del billete
int determinarClase(int costo) {
  if (costo > 1000) return 0;       // Clase alta
  else if (costo >= 500) return 1;  // Clase media
  else return 2;                    // Clase económica
}

// Decide si al pasajero le toca ventana/pasillo (extremo) o centro
bool determinarTipoAsiento(int clase, int edad) {
  if (clase == 0 && edad > 25) return true;  // Mayores de 25 en clase alta, ventana/pasillo
  return (edad >= 15 && edad <= 50);         // Entre 15 y 50, centro.  Otros, lo que haya
}

// Busca un asiento libre
bool asignarAsiento(int clase, bool extremo, int &fila, int &asiento) {

  // Primero busca asientos con la preferencia del pasajero
  for (int f = 0; f < 4; f++) {
    for (int a = 0; a < 3; a++) {
      if (extremo && (a == 0 || a == 2) && !clases[clase][f][a].ocupado) {
        fila = f;
        asiento = a;
        return true;
      } else if (!extremo && a == 1 && !clases[clase][f][a].ocupado) {
        fila = f;
        asiento = a;
        return true;
      }
    }
  }

  // Si no encuentra con preferencia, busca cualquier asiento libre en la clase
  for (int f = 0; f < 4; f++) {
    for (int a = 0; a < 3; a++) {
      if (!clases[clase][f][a].ocupado) {
        fila = f;
        asiento = a;
        return true;
      }
    }
  }

  return false;
}

// Actualiza el buffer con la información actual de los asientos
void updateDisplayBuffer() {
  for (int filaLED = 0; filaLED < 8; filaLED++) {
    uint8_t filaBits = 0;

    if (filaLED < 4) {  // Primeras 4 filas: Clase 0 y Clase 1
      // Clase 0 (primeras 3 columnas)
      for (int a = 0; a < 3; a++) {
        if (clases[0][filaLED][a].ocupado) {
          filaBits |= (1 << a);  // Enciende el LED
        }
      }

      // Clase 1 (últimas 3 columnas)
      for (int a = 0; a < 3; a++) {
        if (clases[1][filaLED][a].ocupado) {
          filaBits |= (1 << (a + 5));  // Enciende el LED, desplazado 5 posiciones
        }
      }
    } else {
      // Filas 4-7: Clase 2.
      int filaClase = filaLED - 4;
      for (int a = 0; a < 3; a++) {
        if (clases[2][filaClase][a].ocupado) {
          filaBits |= (1 << a);  // Enciende el LED
        }
      }
    }
    displayBuffer[filaLED] = filaBits;
  }
}

// Muestra el estado de los asientos en el serial
void imprimirEstadoAsientos() {
  for (int c = 0; c < 3; c++) {
    Serial.print("Clase ");
    Serial.println(c);

    for (int f = 0; f < 4; f++) {
      Serial.print("  Fila ");
      Serial.print(f);
      Serial.print(": ");

      for (int a = 0; a < 3; a++) {
        // Muestra la primera letra del nombre si está ocupado, o "*" si está libre
        Serial.print(clases[c][f][a].ocupado ? clases[c][f][a].nombrePasajero.substring(0, 1) : "*");
        Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println();
  }
}