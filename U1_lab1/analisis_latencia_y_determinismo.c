#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <pigpio.h>

#define GPIO_PIN_LED 4 // Numero de pin GPIO donde esta conectado el LED (GPIO4, pin fisico 7 en la Raspberry Pi)
#define GPIO_PIN_BUTTON 17 // Numero de pin GPIO donde esta conectado el boton (GPIO17, pin fisico 11 en la Raspberry Pi)
#define CANTIDAD_MEDICIONES 20 // Definimos una constante para la cantidad de mediciones que queremos realizar antes de hacer el análisis de latencia y jitter

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

// Variables globales para los datos
uint32_t latencias[CANTIDAD_MEDICIONES];
int pulsaciones = 0;

// BANDERAS (Flags) marcadas como 'volatile'
volatile int flag_imprimir_presionado = 0;
volatile int flag_imprimir_liberado = 0;
volatile int flag_analisis_listo = 0;

/**
 * @brief Manejador de interrupción. Ahora es ultrarrápido y no bloqueante
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

    if (level == 0) // Flanco de bajada (Botón presionado)
    {
        uint32_t tiempo_accion = gpioTick(); // Capturar tiempo INMEDIATAMENTE
        gpioWrite(led, 1);                   // Encender LED INMEDIATAMENTE

        if (pulsaciones < CANTIDAD_MEDICIONES)
        {
            latencias[pulsaciones] = tiempo_accion - tick; // Calcular latencia y guardarla en el arreglo de latencias
            pulsaciones++;
            flag_imprimir_presionado = 1; // Levantamos la bandera en lugar de hacer printf
        }

        if (pulsaciones == CANTIDAD_MEDICIONES)
        {
            flag_analisis_listo = 1; // Levantamos la bandera para que el main haga el cálculo
        }
    }
    else if (level == 1) // Flanco de subida (Botón liberado)
    {
        gpioWrite(led, 0); // Apagar LED INMEDIATAMENTE
        flag_imprimir_liberado = 1; // Levantamos la bandera
    }
}

int main(int argc, char const *argv[])
{
    int pin_led = GPIO_PIN_LED;
    int pin_button = GPIO_PIN_BUTTON;

    // Registrar el manejador de señal para Ctrl+C antes de cualquier operación
    signal(SIGINT, handle_signal);

    if (gpioInitialise() < 0) {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    gpioSetMode(pin_led, PI_OUTPUT); // Configurar el pin del LED como salida
    gpioSetMode(pin_button, PI_INPUT); // Configurar el pin del boton como entrada con resistencia de pull-up
    gpioSetPullUpDown(pin_button, PI_PUD_UP); // Activar resistencia de pull-up interna para el botóns

    // FILTRO DE REBOTE: Ignora pulsaciones menores a 30ms (30,000 us) para evitar falsos positivos
    gpioSetGlitchFilter(pin_button, 30000);

    // Configurar la interrupción para el botón, detectando tanto el flanco de bajada (presión) como el flanco de subida (liberación)
    gpioSetAlertFuncEx(pin_button, manejador_interrupcion_boton, &pin_led);

    // BUCLE PRINCIPAL (El encargado de las tareas lentas y pesadas)
    while(running)
    {
        // Revisar banderas de impresión simple
        if (flag_imprimir_presionado) {
            printf("BOTÓN_PRESIONADO\n");
            flag_imprimir_presionado = 0; // Bajamos la bandera
        }

        if (flag_imprimir_liberado) {
            printf("BOTÓN_LIBERADO\n");
            flag_imprimir_liberado = 0; // Bajamos la bandera
        }

        // Revisar bandera de cálculos matemáticos complejos
        if (flag_analisis_listo)
        {
            printf("\n--- ANÁLISIS DE 20 MUESTRAS TERMINADO ---\n");

            uint32_t latencia_promedio = 0;
            uint32_t maximo = latencias[0];
            uint32_t minimo = latencias[0];

            // Calcular latencia promedio, máximo, mínimo y jitter
            for (int i = 0; i < CANTIDAD_MEDICIONES; i++)
            {
                if (latencias[i] > maximo) maximo = latencias[i];
                if (latencias[i] < minimo) minimo = latencias[i];
                latencia_promedio += latencias[i];
            }

            latencia_promedio /= CANTIDAD_MEDICIONES;
            uint32_t jitter = maximo - minimo;

            printf("Latencias medidas (en microsegundos):\n");
            for (int i = 0; i < CANTIDAD_MEDICIONES; i++) {
                printf("Muestra %d: %u microsegundos\n", i + 1, latencias[i]);
            }

            printf("\nResultados del análisis de latencia:\n");
            printf("Latencia promedio: %u microsegundos\n", latencia_promedio);
            printf("Latencia máxima: %u microsegundos\n", maximo);
            printf("Latencia mínima: %u microsegundos\n", minimo);
            printf("Jitter: %u microsegundos\n", jitter);

            // Reiniciamos las variables para una nueva ronda
            pulsaciones = 0;
            flag_analisis_listo = 0; // Bajamos la bandera
        }

        // Dormir un tiempo muy corto (10ms) para no consumir el 100% de la CPU en este while
        usleep(10000);  // 10ms = 10,000 microsegundos
    }

    gpioTerminate();
    return 0;
}