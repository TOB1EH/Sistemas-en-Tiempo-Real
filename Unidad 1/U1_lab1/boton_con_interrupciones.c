#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <pigpio.h>

#define GPIO_PIN_LED 4 // Numero de pin GPIO donde esta conectado el LED (GPIO4, pin fisico 7 en la Raspberry Pi)
#define GPIO_PIN_BUTTON 17 // Numero de pin GPIO donde esta conectado el boton (GPIO17, pin fisico 11 en la Raspberry Pi)

/**
 * @param sig_atomic_t Tipo especial para trabajar con manejadores de señales.
 * volatile indica al compilador que el valor puede cambiar fuera del flujo principal del programa.
 * Esto es necesario porque la variable es modificada dentro de una función que maneja señales.
 */
volatile sig_atomic_t running = 1;

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

/**
 * @brief Manejador de interrupción para el botón | Esta función se ejecuta cada vez que se detecta un cambio en el estado del botón (presionado o liberado).
 *
 * @param gpio El pin que genero la interrupción (en este caso, el pin del botón).
 * @param level Estado actual del pin del botón (0 para presionado, 1 para liberado).
 * @param tick La marca de tiempo en microsegundos cuando ocurrió la interrupción.
 * @param user_data Puntero a datos del usuario, en este caso se utiliza para pasar el número de pin del LED que se debe controlar.
 * @return * void 
 */
void manejador_interrupcion_boton(int gpio, int level, uint32_t tick, void *user_data)
{
    int led = *(int *)user_data; // Obtener el número de pin del LED desde los datos del usuario

    // Al usar una resistencia de pull-up, el estado normal del boton es HIGH (1)
    // y se vuelve LOW (0) cuando se presiona. Por lo tanto, se verifica si el 
    // nivel es 0 para detectar la presión del botón.

    if (level == 0) // Verificar si el botón fue presionado (nivel bajo)
    {
        gpioWrite(led, 1); // Encender el LED (escribir 1 en el pin del LED)
        printf("BOTÓN_PRESIONADO\n");
    }
    else if (level == 1) // Verificar si el botón fue liberado (nivel alto)
    {
        gpioWrite(led, 0); // Apagar el LED (escribir 0 en el pin del LED)
        printf("BOTÓN_LIBERADO\n");
    }
}

int main(int argc, char const *argv[])
{
    int pin_led = GPIO_PIN_LED;
    int pin_button = GPIO_PIN_BUTTON;

    // Registrar el manejador de señal para Ctrl+C antes de cualquier operación
    signal(SIGINT, handle_signal);

    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    // Configurar el pin del LED como salida
    gpioSetMode(pin_led, PI_OUTPUT);

    // Configurar el pin del boton como entrada con resistencia de pull-up
    gpioSetMode(pin_button, PI_INPUT);

    // Configura el pin del botón con resistencia de pull-up interna para que lea HIGH cuando no se presiona y LOW cuando se presiona
    // Esto asegura que el pin no quede flotando y tenga un estado definido cuando el botón no está siendo presionado
    // La constante PI_PUD_UP indica que se debe activar la resistencia de pull-up interna del pin, lo que hace que el pin lea 
    // HIGH (1) cuando el botón no está presionado y LOW (0) cuando el botón está presionado
    gpioSetPullUpDown(pin_button, PI_PUD_UP);

    // Configurar la interrupción para el botón, detectando tanto el flanco de bajada (presión) como el flanco de subida (liberación)
    // La función manejador_interrupcion_boton se llamará cada vez que se detecte un cambio en el estado del botón, permitiendo manejar ambos eventos (presión y liberación)
    gpioSetAlertFuncEx(pin_button, manejador_interrupcion_boton, &pin_led); // Pasar la dirección del pin del LED como datos del usuario para que el manejador pueda acceder a él

    // Bucle infinito que mantiene el programa vivo
    while(running) {
        usleep(10000);
        // Las interrupciones del botón pausarán este sueño automáticamente.
    }

    // Terminar la libreria pigpio
    gpioTerminate();
    printf("Recursos GPIO finalizados correctamente.\n");
    return 0;
}
