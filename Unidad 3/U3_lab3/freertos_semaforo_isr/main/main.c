#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* ------ Pines ------ */
#define PIN_LED    GPIO_NUM_2   // LED integrado ESP32
#define PIN_BOTON  GPIO_NUM_4   // botón con resistencia pull-up

/* ------ Tags para logs ------ */
static const char* TAG_PROC = "PROCESAMIENTO";
static const char* TAG_TELE = "TELEMETRIA";
static const char* TAG_MAIN = "MAIN";

/* ------ Semáforo global ------ */
static SemaphoreHandle_t sem_boton = NULL;

/* ------ Timestamp del último evento ------ */
static volatile uint32_t ultimo_evento_ms = 0;

/* ================================================
   ISR — se ejecuta cuando el botón es presionado
   REGLA: sin printf, sin delay, sin nada bloqueante
   ================================================ */
static void IRAM_ATTR isr_boton(void* arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    // Dar el semáforo desde la ISR
    xSemaphoreGiveFromISR(sem_boton, &higher_priority_task_woken);

    // Forzar cambio de contexto inmediato si corresponde
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/* ================================================
   Tarea A — Procesamiento (Prioridad 3, Core 1)
   Bloqueada esperando el semáforo de la ISR
   ================================================ */
static void tarea_procesamiento(void* param)
{
    bool estado_led = false;

    while (true)
    {
        // Bloquearse indefinidamente hasta que la ISR
        // entregue el semáforo (equivale al sigwait del lab anterior)
        xSemaphoreTake(sem_boton, portMAX_DELAY);

        // Calcular tiempo desde el último evento
        uint32_t ahora = xTaskGetTickCount() * portTICK_PERIOD_MS;
        uint32_t delta = ahora - ultimo_evento_ms;
        ultimo_evento_ms = ahora;

        // Toggle del LED
        estado_led = !estado_led;
        gpio_set_level(PIN_LED, estado_led ? 1 : 0);

        ESP_LOGI(TAG_PROC,
                 "Evento detectado | Tiempo desde ultimo: %lu ms | LED: %s",
                 (unsigned long)delta,
                 estado_led ? "ON" : "OFF");
    }
}

/* ================================================
   Tarea C — Telemetría (Prioridad 1, Core 1)
   Imprime heartbeat cada 5 segundos
   ================================================ */
static void tarea_telemetria(void* param)
{
    while (true)
    {
        ESP_LOGI(TAG_TELE, "Sistema Operativo Saludable");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ================================================
   Entry point — equivale al main() en Linux
   ================================================ */
void app_main(void)
{
    // Configurar LED como salida
    gpio_reset_pin(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_LED, 0);

    // Configurar botón como entrada con pull-up interno
    // → Lee HIGH en reposo, LOW al presionar
    gpio_reset_pin(PIN_BOTON);
    gpio_set_direction(PIN_BOTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(PIN_BOTON, GPIO_PULLUP_ONLY);

    // Crear semáforo binario ANTES de crear las tareas
    sem_boton = xSemaphoreCreateBinary();
    if (sem_boton == NULL) {
        ESP_LOGE(TAG_MAIN, "Error: no se pudo crear el semáforo");
        return;
    }

    // Instalar el servicio de interrupciones de GPIO
    gpio_install_isr_service(0);

    // Configurar la ISR para flanco de bajada (botón presionado)
    gpio_set_intr_type(PIN_BOTON, GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(PIN_BOTON, isr_boton, NULL);

    // Crear tarea de procesamiento — prioridad alta
    xTaskCreatePinnedToCore(
        tarea_procesamiento,  // función
        "Procesamiento",      // nombre visible en debug
        2048,                 // stack en bytes
        NULL,                 // parámetros
        3,                    // prioridad (3 > 1 → más crítica)
        NULL,                 // handle (no necesario)
        1                     // Core 1
    );

    // Crear tarea de telemetría — prioridad baja
    xTaskCreatePinnedToCore(
        tarea_telemetria,
        "Telemetria",
        2048,
        NULL,
        1,                    // prioridad baja
        NULL,
        1                     // Core 1
    );

    // Inicializar timestamp
    ultimo_evento_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;

    ESP_LOGI(TAG_MAIN, "Sistema iniciado. Presione el botón en GPIO%d", PIN_BOTON);
}