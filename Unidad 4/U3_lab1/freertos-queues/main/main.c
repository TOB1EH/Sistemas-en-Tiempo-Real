#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// Parámetros de configuración del sistema
#define QUEUE_LENGTH 10                  // Capacidad máxima de la cola (cantidad de elementos)
#define QUEUE_ITEM_SIZE sizeof(int)      // Tamaño de cada elemento en la cola (32 bits)
#define SENSOR_MAX_VALUE 100             // Valor máximo que genera el sensor (0-100)
#define PRODUCER_DELAY_MS 500            // Tiempo entre generaciones de valores (ms)
#define CONSUMER_WAIT_TIME_MS 10         // Timeout si la cola está llena (ms)
#define PRODUCER_PRIORITY 2              // Prioridad del productor (mayor = más prioritario)
#define CONSUMER_PRIORITY 1              // Prioridad del consumidor (menor que productor)
#define LED_PIN GPIO_NUM_2               // GPIO 2 para el LED integrado del ESP32

// Cola global para comunicación entre tareas productora y consumidora
QueueHandle_t sensor_queue = NULL;

// Tarea que simula un sensor generando valores y enviándolos a la cola
void tarea_productora(void *params)
{
    int sensor_value;
    BaseType_t queue_status;    // Variable para almacenar el estado de la operación de la cola
    TickType_t timeout_ticks = pdMS_TO_TICKS(CONSUMER_WAIT_TIME_MS); // Convertir tiempo de espera a ticks para FreeRTOS

    while (1)
    {
        // Simular generación de un valor de sensor aleatorio entre 0 y SENSOR_MAX_VALUE
        sensor_value = rand() % (SENSOR_MAX_VALUE + 1);

        printf("[PRODUCTOR] Generando valor del sensor: %d\n", sensor_value);

        // Enviar valor a la cola con timeout. Si la cola está llena, espera hasta timeout_ticks
        queue_status = xQueueSend(sensor_queue, &sensor_value, timeout_ticks);

        // Si el valor se envió exitosamente, imprimir mensaje. Si la cola estaba llena, imprimir advertencia.
        if (queue_status == pdPASS) {
            printf("[PRODUCTOR] Valor %d enviado a la cola exitosamente\n", sensor_value);
        } else if (queue_status == errQUEUE_FULL) {
            printf("[PRODUCTOR] Cola llena. Valor %d descartado tras esperar %d ms\n", 
                   sensor_value, CONSUMER_WAIT_TIME_MS);
        }

        // Simular tiempo entre generaciones de valores del sensor
        vTaskDelay(pdMS_TO_TICKS(PRODUCER_DELAY_MS));
    }
}

// Tarea que consume y procesa los valores recibidos de la cola
void tarea_consumidora(void *params)
{
    int sensor_value;   // Variable para almacenar el valor recibido de la cola
    BaseType_t queue_status; // Variable para almacenar el estado de la operación de la cola

    while (1) {
        // Bloquear hasta que haya un dato disponible en la cola (portMAX_DELAY = espera indefinida)
        queue_status = xQueueReceive(sensor_queue, &sensor_value, portMAX_DELAY);

        // Si se recibió un valor exitosamente, procesarlo. Si la cola estaba vacía (lo cual no debería pasar con portMAX_DELAY), imprimir advertencia.
        if (queue_status == pdPASS) {
            printf("\n[CONSUMIDOR] Dato recibido: %d\n", sensor_value);
            printf("[CONSUMIDOR] Ejecutandose en Core: %d\n", xPortGetCoreID());
            
            // Encender el LED integrado durante el procesamiento
            printf("[CONSUMIDOR] LED ENCENDIDO\n");
            gpio_set_level(LED_PIN, 1);

            printf("[CONSUMIDOR] Procesando...\n");

            // Simular procesamiento del dato con retardo
            vTaskDelay(pdMS_TO_TICKS(100));

            // Apagar el LED al terminar el procesamiento
            printf("[CONSUMIDOR] LED APAGADO\n");
            gpio_set_level(LED_PIN, 0);

            printf("[CONSUMIDOR] Procesamiento completado para valor: %d\n\n", sensor_value);
        }
    }
}

// Punto de entrada de la aplicación en ESP-IDF
void app_main(void)
{
    printf("=== Iniciando Sistema de Telemetria con Queues ===\n");
    
    // Configurar GPIO 2 como salida para controlar el LED integrado
    printf("Configurando LED en GPIO %d\n", LED_PIN);
    
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),      // Seleccionar GPIO 2
        .mode = GPIO_MODE_OUTPUT,                // Configurar como salida
        .pull_up_en = GPIO_PULLUP_DISABLE,       // Sin resistencia de pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,   // Sin resistencia de pull-down
        .intr_type = GPIO_INTR_DISABLE           // Sin interrupciones
    };
    gpio_config(&io_conf);
    gpio_set_level(LED_PIN, 0);  // Iniciar con LED apagado
    
    printf("LED configurado y apagado\n");
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
