#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <mqueue.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <pigpio.h>

#define MPU6050_ADDR  0x68
#define MPU6050_PWR   0x6B  // registro power management
#define MPU6050_ACCEL 0x3B  // registro inicio acelerómetro
#define NOMBRE_COLA "/imu_queue"
#define MAX_MENSAJES 10
#define N_MUESTRAS 10

/* Variables compartidas */
volatile int running = 1;

typedef struct {
    float x;
    float y;
    float z;
} Aceleracion;

typedef struct {
    int i2c_handle;
    mqd_t cola;
} ParametrosHilo;

typedef struct {
    float buffer[N_MUESTRAS];
    int indice;
    float suma;
    int lleno; // Indica si el buffer ya tiene N muestras
} FiltroMediaMovil;

/**
 * @brief Manejador de señales para capturar SIGINT (Ctrl+C) y permitir una salida limpia del programa.
 *
 * @param sig El número de señal capturada (en este caso, SIGINT)
 * @return * void
 */
void handle_signal(int sig) { running = 0; }

/**
 * @brief Inicializa el sensor MPU6050. Despertar el sensor ya que viene dormido por defecto (modo sleep)
 *
 * @param handle El handle del dispositivo I2C obtenido con i2cOpen
 * @return * void
 */
void mpu6050_init(int handle);

/**
 * @brief Lee los datos del sensor MPU6050. Lee los valores de aceleración en los ejes X, Y y Z,
 * los convierte a unidades de g (gravedad) y los almacena en las variables proporcionadas.
 *
 * @param handle El handle del dispositivo I2C obtenido con i2cOpen
 * @param x Puntero a la variable donde se almacenará el valor leído para el eje X
 * @param y Puntero a la variable donde se almacenará el valor leído para el eje Y
 * @param z Puntero a la variable donde se almacenará el valor leído para el eje Z
 * @return * void
 */
void mpu6050_leer(int handle, float* x, float* y, float* z);

/**
 * @brief Función del hilo productor. Este hilo se encarga de leer los datos de aceleración del sensor MPU6050 a una frecuencia de 100 Hz (cada 10 ms) y enviar estos datos a través de una cola de mensajes para que el hilo consumidor los procese. El hilo se ejecuta mientras la variable `running` sea verdadera, lo que permite una salida limpia cuando se recibe una señal SIGINT.
 *
 * @param arg Puntero a una estructura `ParametrosHilo` que contiene el handle del dispositivo I2C y el descriptor de la cola de mensajes. Esta estructura se utiliza para pasar los parámetros necesarios al hilo productor.
 * @return * void* 
 */
void* hilo_productor(void* arg);

/**
 * @brief Función del hilo consumidor. Este hilo se encarga de recibir los datos de aceleración desde la cola de mensajes y procesarlos (en este caso, simplemente imprimirlos en formato CSV). El hilo se ejecuta mientras la variable `running` sea verdadera, lo que permite una salida limpia cuando se recibe una señal SIGINT.
 *
 * @param arg Puntero a una estructura `ParametrosHilo` que contiene el handle del dispositivo I2C y el descriptor de la cola de mensajes. Esta estructura se utiliza para pasar los parámetros necesarios al hilo consumidor.
 * @return * void*
 */
void* hilo_consumidor(void* arg);

/**
 * @brief Función para aplicar un filtro de media móvil a una nueva muestra de datos. Esta función mantiene un buffer circular de las últimas N muestras, calcula la suma de estas muestras y devuelve el promedio. El filtro se utiliza para suavizar los datos de aceleración antes de imprimirlos o enviarlos a Python.
 *
 * @param f Puntero a la estructura FiltroMediaMovil que contiene el buffer y los parámetros del filtro
 * @param nuevo_valor El nuevo valor de aceleración que se desea filtrar
 * @return * float El valor filtrado (promedio de las últimas N muestras) que se puede imprimir o enviar a Python
 */
float filtrar(FiltroMediaMovil* f, float nuevo_valor);

/**
 * @brief Función principal del programa. Inicializa la comunicación I2C con el sensor MPU6050, configura el sensor, y luego entra en un bucle donde lee continuamente los datos de aceleración y los imprime en la consola. El programa se detiene cuando se recibe una señal SIGINT (Ctrl+C).
 *
 * @return * int
 */
int main() {
    // Inicializar la biblioteca pigpio para usar GPIO e I2C
    if (gpioInitialise() < 0)
    {
        printf("Error al inicializar la biblioteca pigpio\n");
        return 1;
    }

    // Abrir el bus I2C 1 y conectar al dispositivo con dirección MPU6050_ADDR
    int handle = i2cOpen(1, MPU6050_ADDR, 0);
    if (handle < 0)
    {
        printf("Error al abrir el bus I2C\n");
        return 1;
    }

    mpu6050_init(handle); // Inicializar el sensor MPU6050

    // Configurar el manejador de señales para capturar SIGINT (Ctrl+C)
    signal(SIGINT, handle_signal);

    // Configurar la cola de mensajes para la comunicación entre hilos
    struct mq_attr attr;
    attr.mq_maxmsg  = MAX_MENSAJES; // número máximo de mensajes en la cola
    attr.mq_msgsize = sizeof(Aceleracion); // tamaño de cada mensaje (struct Aceleracion)
    attr.mq_flags   = 0; // cola bloqueante
    attr.mq_curmsgs = 0; // número actual de mensajes (inicialmente 0)

    // Crear la cola de mensajes con el nombre NOMBRE_COLA y los atributos definidos
    // O_CREAT crea la cola si no existe
    // O_RDWR permite leer y escribir
    mqd_t cola = mq_open(NOMBRE_COLA, O_CREAT | O_RDWR, 0644, &attr);
    if (cola == (mqd_t)-1) {
        fprintf(stderr, "Error al crear la cola\n");
        // cleanup y salir
        gpioTerminate();
        return 1;
    }

    ParametrosHilo args = {handle, cola}; // Estructura para pasar parámetros a los hilos

    // Crear los hilos productor y consumidor
    pthread_t hilo_prod, hilo_cons;
    pthread_create(&hilo_prod, NULL, hilo_productor, &args);
    pthread_create(&hilo_cons, NULL, hilo_consumidor, &args);

    // El hilo principal simplemente espera a que se reciba la señal de terminación (SIGINT) para salir del programa. Mientras tanto, los hilos productor y consumidor se encargan de leer los datos del sensor y procesarlos respectivamente.
    while (running) {
        gpioDelay(100000); // 100ms para esta etapa (después será 10ms)
    }

    // Esperar a que los hilos terminen antes de limpiar recursos y salir
    pthread_join(hilo_prod, NULL);
    pthread_join(hilo_cons, NULL);

    // Limpiar recursos: cerrar la cola de mensajes, eliminarla del sistema, cerrar el handle I2C y terminar la biblioteca pigpio
    mq_close(cola);
    mq_unlink(NOMBRE_COLA); // elimina la cola del sistema
    i2cClose(handle);
    gpioTerminate();
    return 0;
}

void mpu6050_init(int handle)
{
    uint8_t cmd[2] = {MPU6050_PWR, 0x00}; // escribir 0x00 en el registro PWR_MGMT_1 (0x6B)
    i2cWriteDevice(handle, (char*)cmd, 2); // Escribir el byte de datos (0x00) en el registro de dirección (0x6B)
    gpioDelay(10000); // Esperar 10ms para que el sensor se estabilice
}

void mpu6050_leer(int handle, float* x, float* y, float* z)
{
    // Escribir el registro de acelerómetro (0x3B) para preparar la lectura
    uint8_t registro = MPU6050_ACCEL; // dirección del registro de acelerómetro
    i2cWriteDevice(handle, (char*)&registro, 1); // Escribir el byte

    // Leer los 6 bytes de datos del acelerómetro (XH, XL, YH, YL, ZH, ZL)
    uint8_t datos[6]; // Array para almacenar los 6 bytes de datos
    i2cReadDevice(handle, (char*)datos, 6); // Leer 6 bytes de datos del acelerómetro

    // Combinar bytes en int16_t para cada eje. Esto con el fin de obtener el valor completo de 16 bits para cada eje (X, Y, Z)
    int16_t raw_x = (int16_t)(datos[0] << 8 | datos[1]);
    int16_t raw_y = (int16_t)(datos[2] << 8 | datos[3]);
    int16_t raw_z = (int16_t)(datos[4] << 8 | datos[5]);

    // Dividir por 16384.0f para obtener gravedad (g) ya que el rango de aceleración es ±2g y el valor máximo de 16 bits es 32768, por lo que 32768/2 = 16384
    *x = raw_x / 16384.0f;
    *y = raw_y / 16384.0f;
    *z = raw_z / 16384.0f;
}

void* hilo_productor(void* arg)
{
    ParametrosHilo* args = (ParametrosHilo*)arg;

    // 100 Hz = cada 10ms
    // ¿qué mecanismo usamos para temporización precisa sin sleep()?
    // Pista: lo usaste en el lab del servo con gpioTick()
    uint32_t ultimo_tick = gpioTick();

    // El hilo productor se ejecuta en un bucle infinito mientras la variable `running` sea verdadera. Dentro del bucle, se obtiene el tiempo actual utilizando `gpioTick()`, que devuelve el tiempo en microsegundos desde que se inicializó la biblioteca pigpio. Si han pasado al menos 10 ms (10000 µs) desde la última lectura, el hilo lee los datos de aceleración del sensor MPU6050 utilizando la función `mpu6050_leer()`, que llena las variables `x`, `y` y `z` con los valores de aceleración correspondientes a cada eje.
    while (running) {
        // Obtener el tiempo actual en microsegundos utilizando gpioTick(), que devuelve el tiempo transcurrido desde que se inicializó la biblioteca pigpio. Esto se utiliza para controlar la frecuencia de lectura del sensor.
        uint32_t ahora = gpioTick();

        // Si han pasado al menos 10 ms desde la última lectura, se procede a leer los datos del sensor y enviarlos a la cola de mensajes. Esto asegura que el hilo productor lea los datos a una frecuencia de 100 Hz, lo que es adecuado para aplicaciones de monitoreo de movimiento en tiempo real.
        if ((ahora - ultimo_tick) >= 10000) { // 10ms = 10000 µs
            float x, y, z;
            mpu6050_leer(args->i2c_handle, &x, &y, &z); // Leer los datos de aceleración del sensor MPU6050 y almacenarlos en las variables x, y, z

            Aceleracion dato = {x, y, z};
            // Enviar los datos a la cola de mensajes utilizando mq_send(). El mensaje enviado es una estructura Aceleracion que contiene los valores de aceleración para los ejes X, Y y Z. Si la cola está llena (error EAGAIN), se imprime un mensaje de advertencia indicando que la muestra ha sido descartada.
            if (mq_send(args->cola, (char*)&dato, sizeof(dato), 0) < 0) {
                if (errno == EAGAIN)
                    fprintf(stderr, "Cola llena, muestra descartada\n");
            }

            ultimo_tick = ahora;
        }
    }
    return NULL;
}

void* hilo_consumidor(void* arg)
{
    ParametrosHilo* args = (ParametrosHilo*)arg;
    Aceleracion dato;

    // Un filtro por eje — inicializados a cero automáticamente
    FiltroMediaMovil filtro_x = {0};
    FiltroMediaMovil filtro_y = {0};
    FiltroMediaMovil filtro_z = {0};

    // Abrir la misma cola en modo no bloqueante
    mqd_t cola_nb = mq_open(NOMBRE_COLA, O_RDONLY | O_NONBLOCK);

    // El hilo consumidor se ejecuta en un bucle infinito mientras la variable `running` sea verdadera. Dentro del bucle, el hilo intenta recibir un mensaje de la cola de mensajes utilizando `mq_receive()`. Si no hay mensajes disponibles (error EAGAIN), el hilo consumidor espera 1 ms (usleep(1000)) antes de reintentar la lectura. Esto evita que el hilo consumidor consuma CPU innecesariamente mientras espera nuevos datos en la cola de mensajes. Si se recibe un mensaje con éxito, se aplican filtros de media móvil a cada eje de aceleración y luego se imprimen los valores filtrados en formato CSV.
    while (running) {
        // mq_receive bloquea hasta que haya un mensaje disponible
        // por eso el consumidor no quema CPU esperando
        ssize_t bytes = mq_receive(cola_nb,
                                   (char*)&dato,
                                   sizeof(dato),
                                   NULL);
        // Si no hay mensajes disponibles (error EAGAIN), el hilo consumidor espera 1 ms (usleep(1000)) antes de reintentar la lectura. Esto evita que el hilo consumidor consuma CPU innecesariamente mientras espera nuevos datos en la cola de mensajes.
        if (bytes < 0) {
            if (errno == EAGAIN) {
                usleep(1000); // no había mensajes, esperar 1ms y reintentar
                continue;
            }
            continue;
        }
        // Aplicar filtro a cada eje
        float x_f = filtrar(&filtro_x, dato.x);
        float y_f = filtrar(&filtro_y, dato.y);
        float z_f = filtrar(&filtro_z, dato.z);

        // Imprimir filtrado — esto es lo que ve Python
        printf("%.4f,%.4f,%.4f\n", x_f, y_f, z_f);
        fflush(stdout);
    }

    mq_close(cola_nb); // Cerrar el descriptor de la cola no bloqueante
    return NULL;
}

float filtrar(FiltroMediaMovil* f, float nuevo_valor)
{
    f->suma -= f->buffer[f->indice];  // restar el valor más viejo
    f->buffer[f->indice] = nuevo_valor;
    f->suma += nuevo_valor;           // sumar el nuevo
    f->indice = (f->indice + 1) % N_MUESTRAS;

    // contar hasta tener el buffer lleno
    if (!f->lleno && f->indice == 0) f->lleno = 1;

    // dividir por N solo cuando hay N muestras reales
    int divisor = f->lleno ? N_MUESTRAS : f->indice;
    if (divisor == 0) return nuevo_valor;
    return f->suma / divisor;
}