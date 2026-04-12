#define _GNU_SOURCE // Necesario para usar pthread_setschedparam y otras funciones avanzadas de pthreads

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>

// #define SCHEMA_EJECUCION SCHED_OTHER
#define SCHEMA_EJECUCION SCHED_FIFO


/* ----- Variables compartidas ----- */

// Variables para contar iteraciones de cada hilo
volatile long iter_critico    = 0;
volatile long iter_navegacion = 0;
volatile long iter_telemetria = 0;

volatile int programa_activo = 1; // Bandera para controlar la ejecución de los hilos

pthread_mutex_t mutex_recurso = PTHREAD_MUTEX_INITIALIZER; // Mutex para proteger el acceso a recursos compartidos (si es necesario)

/* ----- Metodos de Funciones ----- */

/**
 * @brief Función para el hilo que simula el trabajo crítico del sistema de control de vuelo, como el control de estabilidad o la gestión de motores. Este hilo tiene la prioridad más alta para asegurar que siempre tenga acceso a la CPU cuando necesite ejecutarse, lo que es crucial para mantener la seguridad y estabilidad del vehículo aéreo.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void*
 */
void* tarea_critica(void *arg);

/**
 * @brief Función para el hilo que simula el trabajo de navegación del sistema de control de vuelo, como el cálculo de rutas o la gestión de sensores de navegación. Este hilo tiene una prioridad media para asegurar que pueda ejecutarse con regularidad sin interferir con el trabajo crítico, pero aún así tenga acceso a la CPU cuando sea necesario para mantener la precisión y eficiencia de la navegación.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void*
 */
void* tarea_navegacion(void *arg);

/**
 * @brief Función para el hilo que simula el trabajo de telemetría del sistema de control de vuelo, como la transmisión de datos de telemetría o la gestión de comunicaciones. Este hilo tiene la prioridad más baja para asegurar que pueda ejecutarse cuando no haya otras tareas críticas o de navegación en ejecución, pero aún así tenga acceso a la CPU cuando sea necesario para mantener la comunicación efectiva con el centro de control.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void*
 */

void* tarea_telemetria(void *arg);


/* ----- Funcion Principal ----- */
int main(int argc, char const *argv[])
{
    pthread_t hilo_critico;
    pthread_t hilo_navegacion;
    pthread_t hilo_telemetria;

    // Bloquear SIGALRM para que solo lo reciba el hilo de telemetría
    sigset_t set; // Crear un conjunto de señales vacío
    sigemptyset(&set); // Inicializar el conjunto de señales para que contenga una señal vacia
    sigaddset(&set, SIGALRM); // Agregar la señal SIGALRM al conjunto de señales para bloquearla en el hilo principal y en los hilos de mayor prioridad, lo que asegura que solo el hilo de telemetría reciba esta señal y pueda procesarla sin interferencias de otros hilos
    pthread_sigmask(SIG_BLOCK, &set, NULL); // Bloquear la señal SIGALRM en el hilo principal y en los hilos de mayor prioridad, lo que asegura que solo el hilo de telemetría reciba esta señal y pueda procesarla sin interferencias de otros hilos

    // Timer
    struct itimerval timer; // Estructura para configurar el timer
    timer.it_value.tv_sec    = 0;
    timer.it_value.tv_usec   = 500000;  // 500ms
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL); // Iniciar el timer (ITIMER_REAL utiliza el tiempo real del sistema para generar señales SIGALRM, que serán recibidas por el hilo de telemetría debido a la configuración de bloqueo de señales)

    // Crear el hilo para el trabajo crítico
    pthread_create(&hilo_critico, NULL, tarea_critica, NULL);

    // Crear el hilo para el trabajo de navegación
    pthread_create(&hilo_navegacion, NULL, tarea_navegacion, NULL);

    // Crear el hilo para el trabajo de telemetría
    pthread_create(&hilo_telemetria, NULL, tarea_telemetria, NULL);

    sleep(10); // Simula el tiempo de ejecución del programa

    programa_activo = 0; // Indica a los hilos que deben terminar

    // Esperar a que los hilos terminen
    pthread_join(hilo_critico, NULL);
    pthread_join(hilo_navegacion, NULL);
    pthread_join(hilo_telemetria, NULL);

    printf("Tabla comparativa de iteraciones:\n");
    printf("%-20s %-12s %-15s\n", "Hilo", "Prioridad", "Iteraciones");
    printf("%-20s %-12d %-15ld\n", "Crítico",    80, iter_critico);
    printf("%-20s %-12d %-15ld\n", "Navegación", 40, iter_navegacion);
    printf("%-20s %-12d %-15ld\n", "Telemetría", 10, iter_telemetria);

    return 0;
}

void* tarea_critica(void *arg)
{
    // Configurar prioridad RT para este hilo
    struct sched_param param;
    param.sched_priority = 80;

    pthread_setschedparam(pthread_self(), SCHEMA_EJECUCION, &param);

    // Descomentar para evaluar el inciso C y comentar el resto
    // usleep(100000); // espera 100ms para que telemetría tome el mutex primero

    double resultado = 0.0;
    while (programa_activo)
    {
        // pthread_mutex_lock(&mutex_recurso); // Bloquear el mutex para simular acceso a un recurso compartido crítico, lo que garantiza que este hilo tenga acceso exclusivo a ese recurso mientras realiza sus cálculos intensivos, lo que es crucial para mantener la integridad y seguridad del sistema de control de vuelo
        resultado += iter_critico * 3.14; // Simula un cálculo intensivo
        iter_critico++;
        // pthread_mutex_unlock(&mutex_recurso); // Desbloquear el mutex después de terminar el trabajo crítico para permitir que otros hilos puedan acceder al recurso compartido si es necesario
    }
    return NULL;
}

void* tarea_navegacion(void *arg)
{
    // Configurar prioridad RT para este hilo
    struct sched_param param;
    param.sched_priority = 40;

    pthread_setschedparam(pthread_self(), SCHEMA_EJECUCION, &param);

    double resultado = 0.0;
    while (programa_activo)
    {
        resultado += iter_navegacion * 3.14; // Simula un cálculo intensivo
        iter_navegacion++;
    }
    return NULL;
}

void* tarea_telemetria(void *arg)
{
    // Configurar prioridad RT para este hilo
    struct sched_param param;
    param.sched_priority = 10;

    pthread_setschedparam(pthread_self(), SCHEMA_EJECUCION, &param);

    sigset_t set; // Crear un conjunto de señales vacío
    sigemptyset(&set); // Inicializar el conjunto de señales para que contenga una señal vacia
    sigaddset(&set, SIGALRM); // Agregar la señal SIGALRM al conjunto de señales para bloquearla en este hilo, lo que evita que las interrupciones de telemetría afecten el rendimiento de este hilo de baja prioridad

    double resultado = 0.0;
    while (programa_activo)
    {
        int sig; // Variable para almacenar la señal recibida
        sigwait(&set, &sig); // Esperar a que llegue una señal SIGALRM, lo que simula la recepción de datos de telemetría cada vez que se genera esta

        // Cuando llega SIGALRM, se ejecuta esta parte del código, que simula el procesamiento de los datos de telemetría. Al usar sigwait, este hilo solo se activa cuando llega la señal SIGALRM, lo que permite que los hilos de mayor prioridad (crítico y navegación) tengan más tiempo de CPU para ejecutar sus tareas sin interrupciones frecuentes causadas por la telemetría.
        // pthread_mutex_lock(&mutex_recurso); // Bloquear el mutex para simular acceso a un recurso compartido crítico, lo que garantiza que este hilo tenga acceso exclusivo a ese recurso mientras realiza sus cálculos intensivos, lo que es crucial para mantener la integridad y seguridad del sistema de control de vuelo

        // Trabajo largo mientras lo tiene — crítico queda bloqueado acá
        // for (long i = 0; i < 50000000; i++) {
            // resultado += i * 3.14;
        // }

        // Comentar para evaluar el inciso C y descomentar el resto
        resultado += iter_telemetria * 3.14; // Simula un cálculo intensivo
        iter_telemetria++;

        // pthread_mutex_unlock(&mutex_recurso); // Desbloquear el mutex después de terminar el trabajo de telemetría
    }
    return NULL;
}