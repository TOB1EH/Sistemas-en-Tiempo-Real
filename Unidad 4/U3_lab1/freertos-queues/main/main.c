#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Parámetros de configuración del sistema
#define QUEUE_LENGTH 10                  // Capacidad máxima de la cola (cantidad de elementos)
#define QUEUE_ITEM_SIZE sizeof(int)      // Tamaño de cada elemento en la cola (32 bits)
#define SENSOR_MAX_VALUE 100             // Valor máximo que genera el sensor (0-100)
#define PRODUCER_DELAY_MS 500            // Tiempo entre generaciones de valores (ms)
#define CONSUMER_WAIT_TIME_MS 10         // Timeout si la cola está llena (ms)
#define PRODUCER_PRIORITY 2              // Prioridad del productor (mayor = más prioritario)
#define CONSUMER_PRIORITY 1              // Prioridad del consumidor (menor que productor)

// Cola global para comunicación entre tareas productora y consumidora
QueueHandle_t sensor_queue = NULL;

// Tarea que simula un sensor generando valores y enviándolos a la cola
void tarea_productora(void *params)
{
    int sensor_value;
    BaseType_t queue_status;
    TickType_t timeout_ticks = pdMS_TO_TICKS(CONSUMER_WAIT_TIME_MS);

    while (1) {
        sensor_value = rand() % (SENSOR_MAX_VALUE + 1);

        printf("[PRODUCTOR] Generando valor del sensor: %d\n", sensor_value);

        // Enviar valor a la cola con timeout. Si la cola está llena, espera hasta timeout_ticks
        queue_status = xQueueSend(sensor_queue, &sensor_value, timeout_ticks);

        if (queue_status == pdPASS) {
            printf("[PRODUCTOR] Valor %d enviado a la cola exitosamente\n", sensor_value);
        } else if (queue_status == errQUEUE_FULL) {
            printf("[PRODUCTOR] Cola llena. Valor %d descartado tras esperar %d ms\n", 
                   sensor_value, CONSUMER_WAIT_TIME_MS);
        }

        vTaskDelay(pdMS_TO_TICKS(PRODUCER_DELAY_MS));
    }
}

// Tarea que consume y procesa los valores recibidos de la cola
void tarea_consumidora(void *params)
{
    int sensor_value;
    BaseType_t queue_status;

    while (1) {
        // Bloquear hasta que haya un dato disponible en la cola (portMAX_DELAY = espera indefinida)
        queue_status = xQueueReceive(sensor_queue, &sensor_value, portMAX_DELAY);

        if (queue_status == pdPASS) {
            printf("\n[CONSUMIDOR] Dato recibido: %d\n", sensor_value);
            printf("[CONSUMIDOR] Ejecutandose en Core: %d\n", xPortGetCoreID());
            printf("[CONSUMIDOR] Procesando...\n");

            // Simular procesamiento del dato con retardo
            vTaskDelay(pdMS_TO_TICKS(100));

            printf("[CONSUMIDOR] Procesamiento completado para valor: %d\n\n", sensor_value);
        }
    }
}

// Punto de entrada de la aplicación en ESP-IDF
void app_main(void)
{
    printf("=== Iniciando Sistema de Telemetria con Queues ===\n");
    printf("Creando cola de sensores con capacidad para %d items\n\n", QUEUE_LENGTH);

    // Crear la cola de comunicación entre tareas
    sensor_queue = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

    if (sensor_queue == NULL) {
        printf("Error: No se pudo crear la cola\n");
        return;
    }

    printf("Cola creada exitosamente\n\n");

    // Crear tarea productora en el núcleo 1 con prioridad PRODUCER_PRIORITY
    xTaskCreatePinnedToCore(
        tarea_productora,
        "Productor",
        2048,
        NULL,
        PRODUCER_PRIORITY,
        NULL,
        1
    );

    // Crear tarea consumidora en el núcleo 0 con prioridad CONSUMER_PRIORITY
    // Las tareas se ejecutan en paralelo en diferentes núcleos (ESP32 es dual-core)
    xTaskCreatePinnedToCore(
        tarea_consumidora,
        "Consumidor",
        2048,
        NULL,
        CONSUMER_PRIORITY,
        NULL,
        0
    );

    printf("Tareas creadas y corriendo en sus nucleos asignados\n");
}
