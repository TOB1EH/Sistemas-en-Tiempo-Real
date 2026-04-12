#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <pigpio.h>

// -------------- Pines y constantes --------------
#define GPIO_PIN_LED    4       // Numero de pin GPIO donde esta conectado el led (GPIO4, pin fisico 7 en la Raspberry Pi)
#define GPIO_PIN_BUTTON 17      // Numero de pin GPIO donde esta conectado el boton (GPIO17, pin fisico 11 en la Raspberry Pi)
#define GPIO_PIN_SERVO  27      // Numero de pin GPIO donde esta conectado el servo (GPIO18, pin fisico 12 en la Raspberry Pi)
#define POSICION_MINIMA 500     // Posición mínima del servo en microsegundos (correspondiente a 0 grados)
#define POSICION_MAXIMA 2500    // Posición máxima del servo en microsegundos (correspondiente a 180 grados)
#define INTERVALO_MS    20000   // cada cuántos µs mover el servo (ajustá para suavidad)
#define INCREMENTO      20      // cuántos µs avanza por paso

// -------------- Variables compartidas --------------
volatile sig_atomic_t   running = 1;                        // Bandera para controlar la ejecución del programa
pthread_t               hilo;                               // Variable para almacenar el identificador del hilo
volatile int            posicion_servo = POSICION_MINIMA;   // Variable global para almacenar la posición actual del servo (0 a 180 grados)
volatile int            modo_alerta = 0;                    // Bandera para indicar si estamos en modo de alerta (1) o modo seguro (0)
volatile int            flag_telemetria = 0;                // Bandera para avisar al main que es hora de imprimir la telemetria

// Mutex para proteger el acceso a las variables compartidas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Funcion que simula el trabajo de un hilo que monitorea el estado del botón y cambia el modo de operación a "alerta" cuando se presiona el botón. Este hilo tiene una prioridad alta para asegurar que responda rápidamente a la presión del botón.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void* Un puntero que se devuelve al finalizar la función (no se utiliza en este caso, por lo que se devuelve NULL)
 */
void* funcion_del_hilo(void* arg)
{
    // Configurar prioridad RT para este hilo
    struct sched_param param;
    param.sched_priority = 80; // Prioridad alta (0-99, donde 99 es la más alta ideal para procesos critricos a nivel de kernel, por ende no aplica a este caso)

    // Establecer el planificador a FIFO para garantizar que este hilo tenga prioridad sobre otros hilos y se ejecute completamente antes de que otros hilos puedan ejecutarse, lo que es crucial para responder rápidamente a la presión del botón y cambiar al modo de alerta sin demoras.
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    int alerta_impresa = 0;

    // Simular el trabajo de monitoreo del botón y cambio a modo de alerta
    while (running)
    {

        if (gpioRead(GPIO_PIN_BUTTON) == 0) {
            modo_alerta = 1;
            gpioWrite(GPIO_PIN_LED, 1);
            if (!alerta_impresa) {
                printf("[ALERTA] Parada de emergencia activada - Latencia detectada\n");
                alerta_impresa = 1;
            }
        } else {
            modo_alerta = 0;
            gpioWrite(GPIO_PIN_LED, 0);
            alerta_impresa = 0;  // resetear para la próxima presión
        }
        usleep(10000); // Dormir por 10ms para reducir el uso de CPU, ajusta según la sensibilidad que necesites para detectar el botón
    }

    return NULL;
}

/**
 * @brief Manejador para la señal SIGALRM, que se ejecuta cada vez que el timer configurado con setitimer genera una señal SIGALRM (cada 1 segundo en este caso).
 *
 * @param sig El número de la señal (SIGALRM)
 * @return * void
 */
void handler_telemetria(int sig)
{
    // Esta funcion se ejecuta exactamente cada 1 segundo.
    flag_telemetria = 1; // Levantar la bandera para que el main sepa que es hora de imprimir la telemetria
}

/**
 * @brief Función para iniciar el timer que generará señales SIGALRM cada segundo para la telemetria
 *
 * @return * void
 */
void iniciar_timer()
{
    signal(SIGALRM, handler_telemetria); // Configurar el manejador de señal para la telemetria

    struct itimerval timer;
    timer.it_value.tv_sec = 1; // Primer disparo después de 1 segundo
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1; // Disparos subsecuentes cada 1 segundo
    timer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &timer, NULL); // Iniciar el timer (ITIMER_REAL utiliza el tiempo real del sistema para generar señales SIGALRM)
    // NULL se pasa como tercer argumento porque no necesitamos almacenar el valor anterior del timer.
}

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
    running = 0;
}

/**
 * @brief Codigo principal del programa
 *
 * @param argc El número de argumentos de la línea de comandos (no se utiliza en este programa)
 * @param argv Un array de cadenas que contiene los argumentos de la línea de comandos (no se utiliza en este programa)
 * @return * int El código de salida del programa (0 para indicar que el programa terminó correctamente)
 */
int main(int argc, char const *argv[])
{
    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    // Configurar el pin del botón como entrada
    gpioSetMode(GPIO_PIN_BUTTON, PI_INPUT);

    // Configura el pin del botón con resistencia de pull-up interna para que lea HIGH cuando no se presiona y LOW cuando se presiona
    // Esto asegura que el pin no quede flotando y tenga un estado definido cuando el botón no está siendo presionado
    // La constante PI_PUD_UP indica que se debe activar la resistencia de pull-up interna del pin, lo que hace que el pin lea 
    // HIGH (1) cuando el botón no está presionado y LOW (0) cuando el botón está presionado
    gpioSetPullUpDown(GPIO_PIN_BUTTON, PI_PUD_UP);

    // FILTRO ANTIRREBOTE POR HARDWARE (200,000 microsegundos = 200ms)
    // El hardware filtrará el ruido, manteniendo la CPU libre
    gpioGlitchFilter(GPIO_PIN_BUTTON, 200000);

    // Configurar el pin del LED como salida
    gpioSetMode(GPIO_PIN_LED, PI_OUTPUT);

    // Registrar el manejador de señal para Ctrl+C antes de cualquier operación
    signal(SIGINT, handle_signal);

    // Iniciar el timer para la telemetria
    iniciar_timer();

    // Crear el hilo que simula el trabajo de monitoreo del botón y cambio a modo de alerta
    pthread_create(&hilo, NULL, funcion_del_hilo, NULL);

    // Mover el servo a la posición inicial (500 microsegundos, correspondiente a 0 grados)
    gpioServo(GPIO_PIN_SERVO, POSICION_MINIMA);

    uint32_t ultimo_tick = gpioTick();
    int direccion = 1;

    while(running)
    {
        uint32_t ahora = gpioTick();

        if (!modo_alerta && (ahora - ultimo_tick) >= INTERVALO_MS) { // ¿cada cuánto mover?
            pthread_mutex_lock(&mutex); // Bloquear el mutex antes de acceder a la variable compartida

            posicion_servo += direccion * INCREMENTO; // Cambiar la posición del servo en la dirección actual

            if (posicion_servo >= POSICION_MAXIMA) {
                posicion_servo = POSICION_MAXIMA;  // clamp
                direccion = -1;
            }
            if (posicion_servo <= POSICION_MINIMA) {
                posicion_servo = POSICION_MINIMA;  // clamp
                direccion = 1;
            }

            gpioServo(GPIO_PIN_SERVO, posicion_servo); // Mover el servo a la nueva posición
            pthread_mutex_unlock(&mutex); // Desbloquear el mutex después de acceder a la variable compartida
            ultimo_tick = ahora; // Actualizar el último tick registrado
        }

        if (flag_telemetria)
        {
            // Imprimir la telemetria
            pthread_mutex_lock(&mutex); // Bloquear el mutex antes de acceder a la variable compartida
            printf("[TELEMETRIA] Posicion: %d us | Modo: %s\n",
                    posicion_servo,
                    modo_alerta ? "Alerta" : "Seguro");
            pthread_mutex_unlock(&mutex); // Desbloquear el mutex después de acceder a la variable
            flag_telemetria = 0; // Reiniciar la bandera
        }
    }

    // Cleanup siempre se ejecuta, sin importar por qué salió el loop
    pthread_join(hilo, NULL);
    gpioServo(GPIO_PIN_SERVO, POSICION_MINIMA); // Mover el servo a la posición segura antes de salir
    gpioTerminate();

    printf("Programa finalizado correctamente.\n");

    return 0;
}
