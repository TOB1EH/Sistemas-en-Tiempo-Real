# Implementacion del Sistema de Telemetria con Queues - FreeRTOS ESP32

## Resumen de la Implementacion

Este proyecto implementa un sistema de telemetria de doble nucleo donde se demuestra la comunicacion asincronizada entre tareas mediante queues de FreeRTOS en un ESP32.

## Componentes Principales

### 1. Configuracion (Lineas 7-13)

```c
#define QUEUE_LENGTH 10              /* Capacidad maxima de la cola */
#define QUEUE_ITEM_SIZE sizeof(int)  /* Tamanio de cada elemento */
#define SENSOR_MAX_VALUE 100         /* Valor maximo del sensor simulado */
#define PRODUCER_DELAY_MS 500        /* Frecuencia de ejecucion del productor */
#define CONSUMER_WAIT_TIME_MS 10     /* Timeout para envio a la cola */
#define PRODUCER_PRIORITY 2          /* Prioridad del productor (Media) */
#define CONSUMER_PRIORITY 1          /* Prioridad del consumidor (Baja) */
```

### 2. Variable Global

```c
QueueHandle_t sensor_queue = NULL;   /* Handle de la cola */
```

### 3. Tarea Productora (Lineas 17-39)

**Ubicacion**: Core 1
**Prioridad**: 2 (Media)
**Frecuencia**: Cada 500 ms

Funcionalidad:
- Genera un numero aleatorio entre 0 y 100 simulando lectura de sensor
- Imprime el valor generado
- Intenta enviar el dato a la cola con timeout de 10 ms
- Si la cola esta llena, descarta el dato tras esperar 10 ms
- Si el envio es exitoso, confirma con mensaje

Puntos clave del codigo:
- `rand() % (SENSOR_MAX_VALUE + 1)`: Genera valor aleatorio
- `xQueueSend()`: Envia dato a la cola con timeout
- `pdMS_TO_TICKS()`: Convierte milisegundos a ticks de FreeRTOS
- `vTaskDelay()`: Pausa la ejecucion

### 4. Tarea Consumidora (Lineas 41-59)

**Ubicacion**: Core 0
**Prioridad**: 1 (Baja)
**Comportamiento**: Bloqueada esperando datos

Funcionalidad:
- Permanece bloqueada hasta que hay datos disponibles en la cola
- Al recibir un dato, imprime:
  - El valor recibido
  - El nucleo en el que se ejecuta (xPortGetCoreID())
  - Mensaje "Procesando..." mientras simula carga de trabajo
  - Confirmacion de finalizacion
- Vuelve a bloquearse esperando siguiente dato

Puntos clave del codigo:
- `xQueueReceive()`: Recibe dato de la cola (bloqueante por defecto)
- `portMAX_DELAY`: Espera indefinidamente por un dato
- `xPortGetCoreID()`: Obtiene el ID del nucleo donde se ejecuta (0 o 1)
- `vTaskDelay(pdMS_TO_TICKS(100))`: Simula procesamiento de 100 ms

### 5. Funcion Principal (Lineas 61-96)

La funcion `app_main()`:

1. Imprime encabezado de inicio
2. Crea la cola con capacidad para 10 enteros:
   ```c
   sensor_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);
   ```
3. Verifica que la cola se creo exitosamente
4. Crea la tarea productora en Core 1 con prioridad 2:
   ```c
   xTaskCreatePinnedToCore(
       tarea_productora,      /* Funcion de tarea */
       "Productor",           /* Nombre descriptivo */
       2048,                  /* Stack size en bytes */
       NULL,                  /* Parametros */
       PRODUCER_PRIORITY,     /* Prioridad */
       NULL,                  /* Task handle */
       1                      /* Core afinity (Core 1) */
   );
   ```
5. Crea la tarea consumidora en Core 0 con prioridad 1:
   ```c
   xTaskCreatePinnedToCore(
       tarea_consumidora,     /* Funcion de tarea */
       "Consumidor",          /* Nombre descriptivo */
       2048,                  /* Stack size en bytes */
       NULL,                  /* Parametros */
       CONSUMER_PRIORITY,     /* Prioridad */
       NULL,                  /* Task handle */
       0                      /* Core affinity (Core 0) */
   );
   ```

## Flujo de Ejecucion

1. **Inicio**: app_main() se ejecuta en Core 0, crea la cola y ambas tareas
2. **Productor (Core 1)**:
   - Genera valor aleatorio cada 500 ms
   - Intenta enviar a la cola (espera max 10 ms si esta llena)
   - Si cola esta llena, descarta dato
   - Continua bucle infinito

3. **Consumidor (Core 0)**:
   - Se bloquea en xQueueReceive() esperando dato
   - Cuando hay dato disponible, despierta y lo procesa
   - Imprime informacion del dato y el core actual
   - Simula procesamiento con delay de 100 ms
   - Vuelve a bloquearse

## Comportamiento Esperado en la Terminal

```
=== Iniciando Sistema de Telemetria con Queues ===
Creando cola de sensores con capacidad para 10 items

Cola creada exitosamente

Tareas creadas y corriendo en sus nucleos asignados
[PRODUCTOR] Generando valor del sensor: 45
[PRODUCTOR] Valor 45 enviado a la cola exitosamente

[CONSUMIDOR] Dato recibido: 45
[CONSUMIDOR] Ejecutandose en Core: 0
[CONSUMIDOR] Procesando...
[CONSUMIDOR] Procesamiento completado para valor: 45

[PRODUCTOR] Generando valor del sensor: 78
[PRODUCTOR] Valor 78 enviado a la cola exitosamente
...
```

## Compilacion y Flasheo

### Compilar
```bash
idf.py build
```

### Flashear en ESP32
```bash
idf.py -p /dev/ttyUSB0 flash
```

### Monitorear salida serial (115200 baud)
```bash
idf.py -p /dev/ttyUSB0 monitor
```

Para salir del monitor: `Ctrl+]`

## Conceptos de FreeRTOS Utilizados

1. **Queues**: Mecanismo de comunicacion seguro entre tareas
2. **Tareas**: Unidades de ejecucion con prioridad y stack propio
3. **Core Affinity**: Fijacion de tareas a nucleos especificos
4. **Sincronizacion**: Las tareas se sincronizan mediante la cola
5. **Prioridades**: Controla el orden de ejecucion cuando multiples tareas estan listas
6. **Bloqueantes**: Las tareas se bloquean esperando datos en la cola

## Consideraciones de Disenio

- **Desacoplamiento**: Productor y consumidor no se conocen directamente
- **Seguridad**: La cola maneja acceso concurrente sin riesgo de corrupcion
- **Determinismo**: Comportamiento predecible y controlable
- **Tolerancia a sobrecarga**: Si productor es mas rapido que consumidor, la cola se llena y descarta datos
- **Eficiencia**: Consumidor no consume CPU esperando, queda bloqueado
