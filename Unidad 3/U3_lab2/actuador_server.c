#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

#define SOCKET_PATH  "/tmp/control_led.sock" // Archivo de socket para comunicación local
#define LOG_PATH     "/tmp/alarma.log"  // Archivo de log para registrar cambios de estado
#define MAX_CLIENTES 10
#define BUF_SIZE     64 // Tamaño del buffer para comandos y respuestas

// Estado del sistema (equivale al LED en el enunciado)
typedef struct {
    int     activa;           // 0=OFF, 1=ON
    char    ultima_accion[32]; // hora del último cambio
} EstadoAlarma;

// Estado global compartido por los hilos
EstadoAlarma estado = {0, "nunca"};

// Mutex para proteger el acceso al estado compartido
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

volatile int running  = 1;

/**
 * @brief Registra un mensaje en el archivo de log con timestamp
 * (equivalente a escribir en el GPIO para controlar el LED, pero
 * aquí solo guardamos un registro de la acción)
 *
 * @param mensaje El mensaje a registrar (ej. "ALARMA ACTIVADA" o "ALARMA DESACTIVADA")
 * @return * void
 */
void registrar_log(const char* mensaje)
{
    FILE* f = fopen(LOG_PATH, "a");
    if (!f) return;

    time_t ahora = time(NULL);
    char tiempo[32];
    strftime(tiempo, sizeof(tiempo), "%Y-%m-%d %H:%M:%S", localtime(&ahora));
    fprintf(f, "[%s] %s\n", tiempo, mensaje);
    fclose(f);
}

/**
 * @brief Actualiza el campo `ultima_accion` del estado con la hora actual formateada como HH:MM:SS.
 * Basicamnete actualiza la hora del ultimo cambio.
 * Esta función se llama cada vez que se activa o desactiva la alarma para mantener un registro de
 * cuándo ocurrió el último cambio.
 *
 * @return * void
 */
void actualizar_timestamp()
{
    time_t ahora = time(NULL);
    strftime(estado.ultima_accion,
             sizeof(estado.ultima_accion),
             "%H:%M:%S",
             localtime(&ahora));
}

/**
 * @brief Manejador de señal para SIGINT (Ctrl+C) que cambia la variable `running` a 0, lo que indica al servidor que debe detenerse. Esto permite una terminación limpia del servidor, cerrando el socket y liberando recursos antes de salir.
 *
 * @param sig El número de la señal recibida (en este caso, SIGINT)
 * @return * void
 */
void handle_signal(int sig) { running = 0; }

/**
 * @brief Función que maneja la comunicación con un cliente específico. Lee el comando enviado por
 * el cliente, actualiza el estado de la alarma según el comando (ON, OFF o STATUS), registra la
 * acción en el log y envía una respuesta al cliente. El acceso al estado compartido se protege con
 * un mutex para evitar condiciones de carrera cuando múltiples clientes intentan modificar el
 * estado simultáneamente.
 *
 * @param arg Un puntero a un entero que representa el file descriptor del cliente conectado. Este
 * fd se aloja dinámicamente en el main y se libera dentro de esta función.
 * @return * void*
 */
void* hilo_worker(void* arg)
{
    int cliente_fd = *(int*)arg;
    free(arg); // el fd fue alojado en heap desde el main

    char buf[BUF_SIZE] = {0};
    char respuesta[BUF_SIZE] = {0};

    // Leer el comando del cliente
    ssize_t n = recv(cliente_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        close(cliente_fd);
        return NULL;
    }
    buf[n] = '\0';

    // Eliminar salto de línea si viene
    buf[strcspn(buf, "\n")] = '\0';

    fprintf(stderr, "[SERVER] Comando recibido: '%s'\n", buf);

    // --- Sección crítica ---
    pthread_mutex_lock(&mutex);

    if (strcmp(buf, "ON") == 0) {
        estado.activa = 1;
        actualizar_timestamp();
        registrar_log("ALARMA ACTIVADA");
        fprintf(stderr, "[SERVER] Alarma ON\n");
        snprintf(respuesta, sizeof(respuesta), "LED_OK: ON\n");

    } else if (strcmp(buf, "OFF") == 0) {
        estado.activa = 0;
        actualizar_timestamp();
        registrar_log("ALARMA DESACTIVADA");
        fprintf(stderr, "[SERVER] Alarma OFF\n");
        snprintf(respuesta, sizeof(respuesta), "LED_OK: OFF\n");

    } else if (strcmp(buf, "STATUS") == 0) {
        snprintf(respuesta, sizeof(respuesta),
                 "ESTADO: %s | Ultimo cambio: %s\n",
                 estado.activa ? "ON" : "OFF",
                 estado.ultima_accion);

    } else {
        snprintf(respuesta, sizeof(respuesta),
                 "ERROR: Comando desconocido. Use ON, OFF o STATUS\n");
    }

    pthread_mutex_unlock(&mutex);
    // --- Fin sección crítica ---

    // Responder al cliente
    send(cliente_fd, respuesta, strlen(respuesta), 0);
    close(cliente_fd);
    return NULL;
}

int main(void)
{
    signal(SIGINT, handle_signal);

    // Crear el socket
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Limpiar socket anterior si existía
    unlink(SOCKET_PATH);

    // Configurar dirección
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Bind — vincular el socket al archivo
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // Listen — empezar a escuchar (hasta 5 conexiones en cola)
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    fprintf(stderr, "[SERVER] Servidor iniciado en %s\n", SOCKET_PATH);
    fprintf(stderr, "[SERVER] Esperando clientes...\n");

    // Loop principal — aceptar clientes
    while (running) {
        int* cliente_fd = malloc(sizeof(int));
        *cliente_fd = accept(server_fd, NULL, NULL);

        if (*cliente_fd < 0) {
            free(cliente_fd);
            if (!running) break; // Ctrl+C
            perror("accept");
            continue;
        }

        fprintf(stderr, "[SERVER] Cliente conectado\n");

        // Lanzar hilo para atender a este cliente
        pthread_t hilo;
        pthread_create(&hilo, NULL, hilo_worker, cliente_fd);
        pthread_detach(hilo); // liberar recursos automáticamente al terminar
    }

    // Cleanup
    close(server_fd);
    unlink(SOCKET_PATH);
    fprintf(stderr, "[SERVER] Servidor detenido\n");
    return 0;
}