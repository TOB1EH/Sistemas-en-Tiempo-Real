#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <pigpio.h>


/* ...... Pines GPIO Raspberry PI ------ */
#define AHT10_ADDR 0x38  // dirección I2C del sensor  AHT10
#define GPIO_PIN_VENTILADOR 17 // Número de pin GPIO donde está conectado el ventilador (GPIO17, pin físico 11 en la Raspberry Pi)

/* ------ Variables Compartidas ------ */
volatile sig_atomic_t running = 1; // Bandera para controlar la ejecución del programa

volatile float temperatura_global = 0.0; // Variable global para almacenar la temperatura medida por el sensor AHT10
// int i2c_handle = -1;                     // handle del dispositivo I2C para el sensor AHT10

// Variables para medir el tiempo
volatile uint32_t tick_inicio_alerta      = 0;
volatile uint32_t tick_inicio_ventilacion = 0;

// Enum para manejar los estados del sistema de control de clima
typedef enum {
    REPOSO,
    ALERTA,
    VENTILACION
} EstadoSistema;

volatile EstadoSistema estado_actual = REPOSO; // Variable global para almacenar el estado actual del sistema de control de clima

// Mutex para proteger el acceso a las variables compartidas
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Manejador para la señal SIGINT (Ctrl+C)
 *
 * @param sig El número de la señal (SIGINT para Ctrl+C)
 * @return * void
 */
void handle_signal(int sig);

/**
 * @brief Inicializa el sensor AHT10 para medir temperatura
 * 
 * @param handle El identificador del dispositivo I2C para el sensor AHT10
 * @return * void
 */
void aht10_init(int handle);

/**
 * @brief Pide una medición al sensor AHT10)
 *
 * @param handle El identificador del dispositivo I2C para el sensor AHT10
 * @return * void
 */
void aht10_trigger_measurement(int handle);

/**
 * @brief Lee la temperatura medida por el sensor AHT10 y la devuelve en grados Celsius
 *
 * @param handle El identificador del dispositivo I2C para el sensor AHT10
 * @return * float La temperatura medida por el sensor AHT10 en grados Celsius
 */
float aht10_read_temperature(int handle);

/**
 * @brief Función que simula el trabajo de un hilo que se encarga de adquirir datos del sensor AHT10 y actualizar la variable global con la temperatura medida. Este hilo tiene una prioridad alta para asegurar que adquiera los datos de forma oportuna y actualice la variable global sin demoras significativas.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void*
 */
void* tarea_adquisicion_datos(void* arg);

/**
 * @brief Función que simula el trabajo de un hilo que se encarga de controlar la lógica del sistema de control de clima, tomando decisiones basadas en la temperatura medida por el sensor AHT10. Este hilo tiene una prioridad media para asegurar que controle la lógica del sistema de forma oportuna, pero sin interferir con la adquisición de datos del sensor.
 *
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void* 
 */
void* tarea_control_logico(void* arg);

/**
 * @brief Función que simula el trabajo de un hilo que se encarga de imprimir información de diagnóstico en la consola, como la temperatura actual medida por el sensor AHT10. Este hilo tiene una prioridad baja para asegurar que no interfiera con las tareas críticas de adquisición de datos y control lógico, pero aún así proporciona información útil para el diagnóstico del sistema.
 * 
 * @param arg Un puntero a datos del usuario (no se utiliza en esta función, por lo que se puede pasar NULL)
 * @return * void* 
 */
void* tarea_interfaz_diagnostico(void* arg);

/* ------ Funcion Principal Main ------ */
int main(int argc, char const *argv[])
{
    // Inicializar la libreria pigpio
    if (gpioInitialise() < 0)
    {
        printf("Falla de inicialización de la biblioteca pigpio\n");
        exit(1);
    }

    // Abrir el dispositivo I2C para el sensor AHT10
    int handle = i2cOpen(1, AHT10_ADDR, 0); // El primer argumento es el número del bus I2C (1 para Raspberry Pi), el segundo es la dirección del dispositivo, y el tercero son las opciones (0 para sin opciones)
    if (handle < 0)
    {
        printf("Falla al abrir el dispositivo I2C para el AHT10\n");
        gpioTerminate();
        exit(1);
    }

    signal(SIGINT, handle_signal); // Registrar el manejador de señal para Ctrl+C antes de cualquier operación

    // Inicializar el sensor AHT10
    aht10_init(handle);

    // Configurar el pin del ventilador como salida y asegurarse de que arranque apagado
    gpioSetMode(GPIO_PIN_VENTILADOR, PI_OUTPUT);
    gpioWrite(GPIO_PIN_VENTILADOR, 0);  // asegurar que arranca apagado

    // Crear los tres hilos para las tareas:
    pthread_t hilo_adquisicion, hilo_control, hilo_diagnostico;
    pthread_create(&hilo_adquisicion, NULL, tarea_adquisicion_datos, (void *)handle);
    pthread_create(&hilo_control, NULL, tarea_control_logico, NULL);
    pthread_create(&hilo_diagnostico, NULL, tarea_interfaz_diagnostico, NULL);


    // Esperar hasta la señak que finalice la ejecucion de los hilos (Ctrl + C)
    pthread_join(hilo_adquisicion, NULL);
    pthread_join(hilo_control, NULL);
    pthread_join(hilo_diagnostico, NULL);

    // Cerrar el dispositivo I2C y limpiar la librería pigpio antes de salir
    i2cClose(handle);
    gpioTerminate();

    return 0;
}

void handle_signal(int sig) { running = 0; }

void aht10_init(int handle)
{
    // Comando de inicializacion del AHT10
    uint8_t comando[3] = {0xBE, 0x08, 0x00};
    i2cWriteDevice(handle, (char *)comando, 3); // Enviar el comando de inicialización al sensor AHT10

    gpioDelay(100000); // Esperar 100 ms para que el sensor se inicialice correctamente
}

void aht10_trigger_measurement(int handle)
{
    // Comando para iniciar una medición en el AHT10
    uint8_t comando[3] = {0xAC, 0x33, 0x00};
    i2cWriteDevice(handle, (char *)comando, 3); // Enviar el comando para iniciar la medición al sensor AHT10

    gpioDelay(80000); // El sensor tarda aproximadamente 80 ms en realizar la medición, así que esperamos ese tiempo antes de intentar leer los datos.
}

float aht10_read_temperature(int handle)
{
    /**
     * El sensor AHT10 devuelve 6 bytes de datos después de realizar una medición:
     * Byte 0: Status (bit 7 indica si la medición está lista, bit 6 indica si el sensor está calibrado)
     * Byte 1-2: Datos de humedad (20 bits, pero solo usaremos los 16 bits más significativos para esta práctica)
     * Byte 3-5: Datos de temperatura (20 bits, pero solo usaremos los 16 bits más significativos para esta práctica)
     * Entonces tenemos:
     *  Byte 0: estado del sensor
     *  Byte 1: humedad [19:12]
     *  Byte 2: humedad [11:4]
     *  Byte 3: humedad [3:0] | temperatura [19:16]
     *  Byte 4: temperatura [15:8]
     *  Byte 5: temperatura [7:0]
     */
    uint8_t datos[6];
    i2cReadDevice(handle, (char *)datos, 6); // Leer los 6 bytes de datos del sensor AHT10

    // Extraer los 20 bits de temperatura de los bytes 3, 4 y 5
    uint32_t temp_raw = ((uint32_t)(datos[3] & 0x0F) << 16)
                       | ((uint32_t)datos[4] << 8)
                       | datos[5];

    // Comvertir a Celsius usando la fórmula del datasheet:
    // Temperatura (°C) = (temp_raw / 2^20) * 200 - 50
    float temperatura = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;

    return temperatura;
}

void* tarea_adquisicion_datos(void* arg)
{
    int handle = (int)(intptr_t)arg; // Convertir el puntero de argumento a un entero para obtener el handle del dispositivo I2C

    // Configurar la prioridad del hilo para la tarea A
    struct sched_param param;
    param.sched_priority = 80; // Prioridad alta (0-99, donde 99 es la más alta ideal para procesos críticos a nivel de kernel, por ende no aplica a este caso)
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    while (running)
    {
        // Pedir una medición al sensor AHT10
        aht10_trigger_measurement(handle);

        // Leer la temperatura medida por el sensor AHT10
        float temperatura = aht10_read_temperature(handle);

        // Proteger el acceso a la variable global con un mutex para evitar condiciones de carrera
        pthread_mutex_lock(&mutex);
        temperatura_global = temperatura; // Actualizar la variable global con la nueva temperatura medida
        pthread_mutex_unlock(&mutex);

        gpioDelay(1000000); // Esperar 1 segundo antes de realizar la siguiente medición
    }

    return NULL;
}

void* tarea_control_logico(void* arg)
{
    // Configurar la prioridad del hilo para la tarea B
    struct sched_param param;
    param.sched_priority = 40; // Prioridad media
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    while (running)
    {
        // Leer la temperatura actual de la variable global protegida por mutex
        pthread_mutex_lock(&mutex);
        float temperatura = temperatura_global;
        pthread_mutex_unlock(&mutex);

        uint32_t ahora = gpioTick(); // Obtener el tiempo actual en milisegundos desde que se inició la librería pigpio

        switch (estado_actual)
        {
        case REPOSO:
            // Si la temperatura supera los 30°C, cambiar al estado de ALERTA
            if (temperatura > 24.0f)
            {
                tick_inicio_alerta = ahora; // Registrar el tiempo de inicio de la alerta
                estado_actual = ALERTA;
                printf("[CONTROL] Estado: ALERTA - Temperatura = %.2f °C\n", temperatura);
            }
            break;
        case ALERTA:
            if (temperatura < 24.0f)
            {
                // Como se enfrio, cancelar alerta
                estado_actual = REPOSO;
                printf("[CONTROL] Estado: REPOSO - Temperatura bajó = %.2f °C\n", temperatura);
            }
            else if(ahora - tick_inicio_alerta >= 60000000) // Si han pasado 60 segundos desde que se inició la alerta y la temperatura sigue alta, cambiar al estado de VENTILACION
            {
                tick_inicio_ventilacion = ahora; // Registrar el tiempo de inicio de la ventilación
                estado_actual = VENTILACION;
                gpioWrite(GPIO_PIN_VENTILADOR, 1); // Encender el ventilador
                printf("[CONTROL] Estado: VENTILACION - Temperatura sigue alta = %.2f °C\n", temperatura);
            }
            break;
        case VENTILACION:
            if (temperatura < 20.0f || (ahora - tick_inicio_ventilacion) >= 120000000) // Si la temperatura baja de 25°C o han pasado 120 segundos desde que se inició la ventilación, apagar el ventilador y volver al estado de REPOSO
            {
                gpioWrite(GPIO_PIN_VENTILADOR, 0); // Apagar el ventilador
                estado_actual = REPOSO;
                printf("[CONTROL] Estado: REPOSO - Temperatura bajó o tiempo máximo de ventilación alcanzado = %.2f °C\n", temperatura);
            }
            break;
        }
        // Revisar cada 500ms
        gpioDelay(500000); // Esperar 500 ms antes de volver a verificar la temperatura
    }

    return NULL;
}

void* tarea_interfaz_diagnostico(void* arg)
{
    // Prioridad SCHED_OTHER (prioridad por defecto, más baja que SCHED_FIFO)

    const char* nombres_estado[] = {"REPOSO", "ALERTA", "VENTILACION"};

    while (running)
    {
        // Imprimir la temperatura actual cada 2 segundos
        pthread_mutex_lock(&mutex);
        float temperatura = temperatura_global; // Leer la temperatura actual de la variable global
        EstadoSistema estado = estado_actual; // Leer el estado actual del sistema de control de clima
        pthread_mutex_unlock(&mutex);

        printf("[DIAGNOSTICO] Temperatura actual: %.2f °C | Estado: %s\n", temperatura, nombres_estado[estado_actual]);

        gpioDelay(5000000); // Esperar 5 segundos antes de imprimir la siguiente temperatura
    }
    return NULL;
}