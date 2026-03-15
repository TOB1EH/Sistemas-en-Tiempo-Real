#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pigpio.h>

#define GPIO_PIN_LED 4 // Numero de pin GPIO donde esta conectado el LED (GPIO4, pin fisico 7 en la Raspberry Pi)

uint32_t tiempo_guardado;
uint32_t tiempo_actual;

int main(int argc, char const *argv[])
{
    int pin_led = GPIO_PIN_LED; // Numero de pin GPIO donde esta conectado el LED
    int estado_led = 0; // Variable de estado del LED, 0 para apagado y 1 para encendido

    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    tiempo_guardado = gpioTick(); // Guardar el tiempo inicial en microsegundos para la comparación posterior

    // Configurar el pin del LED como salida
    gpioSetMode(pin_led, PI_OUTPUT);

    while (1)
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
    return 0;
}
