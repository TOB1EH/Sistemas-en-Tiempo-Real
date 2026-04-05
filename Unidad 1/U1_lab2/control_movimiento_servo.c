#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <pigpio.h>

#define GPIO_PIN_BUTTON 17 // Numero de pin GPIO donde esta conectado el boton (GPIO17, pin fisico 11 en la Raspberry Pi)
#define GPIO_PIN_SERVO 27  // Numero de pin GPIO donde esta conectado el servo (GPIO18, pin fisico 12 en la Raspberry Pi)
#define POSICION_MINIMA 500  // Posición mínima del servo en microsegundos (correspondiente a 0 grados)
#define POSICION_MAXIMA 2498 // Posición máxima del servo en microsegundos (correspondiente a 180 grados (Serian 2500 ms pero el calculo no es exacto por ende se desprecias numeros) )
#define SALTOS_POSICION 333 // Incremento en microsegundos para cada cambio de posición (correspondiente a aproximadamente 30 grados)

int posicion_servo = POSICION_MINIMA; // Variable global para almacenar la posición actual del servo (0 a 180 grados)

// Variables volátiles para comunicación segura entre ISR y main
volatile sig_atomic_t running = 1;
volatile int flag_imprimir_estado = 0; // Bandera para avisar al main

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
    gpioServo(GPIO_PIN_SERVO, 0); // Detener el servo (posición 0 para apagar el servo)
    running = 0;
}

/**
 * @brief Manejador de interrupción para el botón | Esta función se ejecuta cada vez que se detecta un cambio en el estado del botón (presionado o liberado).
 *
 * @param gpio El pin que genero la interrupción (en este caso, el pin del botón).
 * @param level Estado actual del pin del botón (0 para presionado, 1 para liberado).
 * @param tick La marca de tiempo en microsegundos cuando ocurrió la interrupción.
 * @return * void
 */
void manejador_interrupcion_boton(int gpio, int level, uint32_t tick)
{
    if (level == 1) // Flanco de subida
    {
        if (posicion_servo >= POSICION_MAXIMA)
            posicion_servo = POSICION_MINIMA;
        else
            posicion_servo += SALTOS_POSICION;

        gpioServo(GPIO_PIN_SERVO, posicion_servo);

        // Levantamos la bandera para imprimir el estado del servo en el main, evitando hacer printf dentro de la interrupción
        flag_imprimir_estado = 1;
    }

    // // Antirrebote (Debounce) de 200ms (200,000 microsegundos)
    // if ((tick - ultimo_tick) > 200000) // Verificar si han pasado al menos 200 ms desde la última interrupción para evitar rebotes
    // {
    //     if (level == 1) // Verificar si el botón fue presionado (nivel alto)
    //     {
    //         if (posicion_servo >= POSICION_MAXIMA) // Verificar si la posición excede el máximo permitido
    //             posicion_servo = POSICION_MINIMA; // Reiniciar a la posición mínima si se excede el máximo
    //         else
    //             posicion_servo += SALTOS_POSICION; // Incrementar la posición del servo

    //         gpioServo(GPIO_PIN_SERVO, posicion_servo); // Mover el servo a la nueva posición

    //         printf("Botón presionado. Posición del servo: %d microsegundos\n", posicion_servo);

    //         ultimo_tick = tick; // Actualizar el último tick registrado
    //     }
    // }
}

int main(int argc, char const *argv[])
{
    int pin_button = GPIO_PIN_BUTTON;

    // Registrar el manejador de señal para Ctrl+C antes de cualquier operación
    signal(SIGINT, handle_signal);

    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    // Configurar el pin del botón como entrada
    gpioSetMode(pin_button, PI_INPUT);

    // Configurar resistencia de pull-down para asegurar que el pin lea 0 cuando el botón no esté presionado
    gpioSetPullUpDown(pin_button, PI_PUD_DOWN);

    // FILTRO ANTIRREBOTE POR HARDWARE (200,000 microsegundos = 200ms)
    // El hardware filtrará el ruido, manteniendo la CPU libre
    gpioSetGlitchFilter(pin_button, 200000);

    // Configurar la función de alerta para el pin del botón, que se ejecutará cada vez que se detecte un
    // cambio en el estado del botón.
    gpioSetAlertFunc(pin_button, manejador_interrupcion_boton);

    gpioServo(GPIO_PIN_SERVO, POSICION_MINIMA); // Inicializar el servo en la posición mínima (0 grados)

    // Bucle infinito que mantiene el programa vivo
    while(running)
    {
        // El main revisa si la interrupción dejó trabajo pendiente (imprimir)
        if (flag_imprimir_estado == 1)
        {
            printf("Botón presionado. Posición del servo: %d microsegundos\n", posicion_servo);
            flag_imprimir_estado = 0; // Bajamos la bandera
        }
        usleep(10000); // Pequeña espera para reducir el uso de CPU, ya que el trabajo principal se hace en la interrupción
    }

    gpioTerminate(); // Terminar la libreria pigpio
    return 0;
}
