#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

/**
 * Simulador de semáforo de tránsito con botón de cruce peatonal.
 * Se utiliza un Semáforo POSIX (sem_t) para sincronizar la señal
 * del botón de cruce entre el hilo de entrada y el hilo principal.
 */

// Semáforo POSIX para el botón de cruce peatonal
sem_t sem_boton;

// Estados del semáforo de tránsito
enum Estado { VERDE, AMARILLO, ROJO };

const char *nombre_estado[] = {"VERDE", "AMARILLO", "ROJO"};
const int duracion_estado[] = {5, 2, 5}; // segundos

// Hilo que detecta la pulsación del botón de cruce peatonal
void *hilo_boton(void *arg)
{
    char c;
    while (1) // Bucle infinito para detectar el botón continuamente
    {
        c = getchar(); // Lee un carácter de la entrada estándar (espera a que el usuario presione 'b' + Enter)
        if (c == 'b' || c == 'B')
        {
            sem_post(&sem_boton); // Señaliza que se presionó el botón
            printf("\n[Botón de cruce peatonal presionado]\n");
        }
    }
    return NULL;
}

// Espera los segundos indicados, verificando cada segundo si el botón fue presionado
int esperar_con_verificacion(int segundos)
{
    for (int i = 0; i < segundos; i++)
    {
        // Verifica si el botón fue presionado sin bloquear el hilo principal
        if (sem_trywait(&sem_boton) == 0)
        {
            return 1; // Botón fue presionado
        }
        sleep(1); // Espera un segundo antes de verificar nuevamente
    }
    return 0;
}

// Activa el cruce peatonal
void cruce_peatonal(void)
{
    printf("\n===== CRUCE PEATONAL ACTIVADO =====\n");
    printf("Semáforo vehicular: ROJO\n");
    printf("Señal peatonal: CRUCE HABILITADO\n");
    sleep(5);
    printf("Señal peatonal: INTERMITENTE (apúrese)\n");
    sleep(3);
    printf("Señal peatonal: NO CRUZAR\n");
    printf("===== FIN CRUCE PEATONAL =====\n\n");
}

int main(int argc, char const *argv[])
{
    pthread_t tid_boton; // Hilo para detectar el botón de cruce peatonal

    /**
     * Inicializar semáforo POSIX: 
     * sem_init(semaforo_que_se_va_a_inicializa, 0 = compartido entre hilos del mismo proceso, valor_inicial_del_semaforo); 
     */
    sem_init(&sem_boton, 0, 0);

    /**
     * Crear hilo para detectar el botón de cruce peatonal:
     * pthread_create(&id_del_hilo, atributos_del_hilo, función_a_ejecutar_por_el_hilo, argumentos_para_la_función_del_hilo (NULL porque la funcion no tiene argumentos));
     */
    pthread_create(&tid_boton, NULL, hilo_boton, NULL);

    printf("Simulador de semáforo iniciado.\n");
    printf("Presione 'b' + Enter para activar el cruce peatonal.\n\n");

    // Estado inicial del semáforo
    enum Estado estado = VERDE;

    while (1) // Bucle infinito para el ciclo del semáforo
    {
        printf("Semáforo: %s (%d segundos)\n", nombre_estado[estado], duracion_estado[estado]);

        // Esperar la duración del estado actual, verificando si el botón fue presionado
        int boton = esperar_con_verificacion(duracion_estado[estado]);

        if (boton) // Si el botón fue presionado, activar el cruce peatonal
        {
            // Si estamos en VERDE, primero pasar por AMARILLO antes del cruce
            if (estado == VERDE)
            {
                printf("Semáforo: AMARILLO (2 segundos)\n");
                sleep(2);
            }
            cruce_peatonal();
            estado = VERDE; // Reiniciar ciclo después del cruce
            continue;
        }

        // Avanzar al siguiente estado del ciclo normal
        switch (estado)
        {
        case VERDE:
            estado = AMARILLO;
            break;
        case AMARILLO:
            estado = ROJO;
            break;
        case ROJO:
            estado = VERDE;
            break;
        }
    }

    // Limpieza (no se alcanza en este bucle infinito)
    sem_destroy(&sem_boton);

    return 0;
}
