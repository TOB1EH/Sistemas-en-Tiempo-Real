#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pigpio.h>

#define GPIO_PIN_LED 4 // Numero de pin GPIO donde esta conectado el LED (GPIO4, pin fisico 7 en la Raspberry Pi)

/**
 * @param sig_atomic_t Tipo especial para trabajar con manejadores de señales.
 * volatile indica al compilador que el valor puede cambiar fuera del flujo principal del programa.
 * Esto es necesario porque la variable es modificada dentro de una función que maneja señales.
 */
volatile sig_atomic_t running = 1;

uint32_t tiempo_guardado;
uint32_t tiempo_actual;

/**
 * @brief Manejador para la señal SIGINT (Ctrl+C)
 * 
 * Permite al programa limpiar recursos (GPIO) de forma ordenada cuando el usuario 
 * presiona Ctrl+C, en lugar de terminar abruptamente.
 * 
 * @param signal El número de la señal (SIGINT para Ctrl+C)
 */
void handle_signal(int signal)
{
    printf("\n\nPrograma interrumpido por el usuario. Finalizando...\n");
    running = 0;
}

int main(int argc, char const *argv[])
{
    int pin_led = GPIO_PIN_LED; // Numero de pin GPIO donde esta conectado el LED
    int estado_led = 0; // Variable de estado del LED, 0 para apagado y 1 para encendido

    // Registrar el manejador de señal para Ctrl+C antes de cualquier operación
    signal(SIGINT, handle_signal);

    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    tiempo_guardado = gpioTick(); // Guardar el tiempo inicial en microsegundos para la comparación posterior

    // Configurar el pin del LED como salida
    gpioSetMode(pin_led, PI_OUTPUT);

    while (running)
    {
        tiempo_actual = gpioTick(); // Mirar el tiempo actual en microsegundos

        if ((tiempo_actual - tiempo_guardado) >= 500000) // Chequear si han pasado 500,000 microsegundos (0.5 segundos) desde la última vez que se cambió el estado del LED
        {
            if (estado_led == 0)
            {
                estado_led = 1; // Si el LED estaba apagado, encenderlo
                printf("LED_ON\n");
            }
            else
            {
                estado_led = 0; // Si el LED estaba encendido, apagarlo
                printf("LED_OFF\n");
            }

            gpioWrite(pin_led, estado_led); // Escribir el nuevo estado del LED en el pin correspondiente (1 para encendido, 0 para apagado)

            tiempo_guardado = tiempo_actual; // Actualizar el tiempo guardado para la próxima comparación
        }
    }

    // Terminar la libreria pigpio
    gpioTerminate();
    printf("Recursos GPIO finalizados correctamente.\n");
    return 0;
}
