# Comunicación Inalámbrica ESP32 con ESP-NOW (Unidireccional)

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

Sistema de comunicación unidireccional entre dispositivos ESP32 utilizando el protocolo ESP-NOW. Incluye un emisor que genera datos estructurados y un receptor que los procesa y muestra.

![demo.gif](assets/demo.gif)

![demo2.gif](assets/demo2.gif)

### Proceso en el Emisor
1. Crea paquetes cada 500-3000ms (intervalo aleatorio)
2. Almacena hasta 5 paquetes en buffer (FIFO). Descarta nuevos datos si el buffer está lleno.
3. Envía los paquetes por ESP-NOW al receptor registrado .
### Proceso en el Receptor
1. Permanece en modo standby esperando paquetes
2. Procesa los datos automáticamente al detectar transmisión
3. Muestra los datos por puerto serial